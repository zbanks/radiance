#!/usr/bin/python

import pygame
from recorder import Recorder
from pll import PLL
from seq import Sequencer,PrintingOutputAdapter
from pattern import Pattern
import time
import numpy
import scipy.signal

class Beat:

	KEY_TAP=pygame.K_SPACE
	KEY_PHASE=pygame.K_RETURN

	KEYBOARD={
		pygame.K_q:(0,0),
		pygame.K_w:(1,0),
		pygame.K_e:(2,0),
		pygame.K_r:(3,0),
		pygame.K_t:(4,0),
		pygame.K_y:(5,0),
		pygame.K_u:(6,0),
		pygame.K_i:(7,0),
		pygame.K_o:(8,0),
		pygame.K_p:(9,0),
		pygame.K_a:(0,1),
		pygame.K_a:(1,1),
		pygame.K_d:(2,1),
		pygame.K_f:(3,1),
		pygame.K_g:(4,1),
		pygame.K_h:(5,1),
		pygame.K_j:(6,1),
		pygame.K_k:(7,1),
		pygame.K_l:(8,1),
		pygame.K_SEMICOLON:(9,1),
		pygame.K_z:(0,2),
		pygame.K_x:(1,2),
		pygame.K_c:(2,2),
		pygame.K_v:(3,2),
		pygame.K_b:(4,2),
		pygame.K_n:(5,2),
		pygame.K_m:(6,2),
		pygame.K_COMMA:(7,2),
		pygame.K_PERIOD:(8,2),
		pygame.K_SLASH:(9,2),
	}


	def __init__(self):
		pygame.init()
		self.size=1024,480
		self.screen=pygame.display.set_mode(self.size)
		self.bg=0,0,0
		self.fg=255,255,255
		self.audio_color=0,0,255

		self.msg=''
		
		self.first_click=0
		self.last_click=0
		self.clicks=0
		self.last_tick=0

		self.r=Recorder()
		self.pll=PLL()
		self.pll.register_tickfn(self.tick)
		self.seq=Sequencer(self.pll,PrintingOutputAdapter())

		self.r.start()
		self.pll.start()
		self.seq.start()


		while True:
			quit=False
			for event in pygame.event.get():
				if event.type==pygame.QUIT: quit=True
				if event.type==pygame.KEYDOWN:
					if event.key==self.KEY_TAP:
						self.tap()
					elif event.key==self.KEY_PHASE:
						self.phase()
					elif event.key==pygame.K_UP:
						self.tempo_adjust(1.)
					elif event.key==pygame.K_DOWN:
						self.tempo_adjust(-1.)
					elif event.key in self.KEYBOARD:
						self.effect(self.KEYBOARD[event.key],True)
				elif event.type==pygame.KEYUP:
					if event.key in self.KEYBOARD:
						self.effect(self.KEYBOARD[event.key],False)
			if quit:
				break

			font = pygame.font.Font(None, 36)
			text = font.render("TEMPO:"+str(self.pll.get_tempo())+" BPM", 1, self.fg)
			text2 = font.render(self.msg, 1, self.fg)

			audio=self.audio_widget()
			pattern_timeline=self.pattern_timeline_widget()

			self.screen.fill(self.bg)
			self.screen.blit(text, (0,0))
			self.screen.blit(audio, (0,60))
			self.screen.blit(pattern_timeline, (audio.get_width(),60))

			pygame.display.flip()

		self.seq.stop()
		self.r.stop()
		self.pll.stop()
		self.seq.join()
		self.r.join()
		self.pll.join()

	def audio_widget(self,height=100):
		data=self.r.get_chunks()

		audio=pygame.Surface((self.r.CAPTURE_CHUNKS,height))

		colors=[(0,0,255),(100,100,255),(200,200,255)]
		beatline_color=(255,0,0)
		for j in range(len(self.r.RANGES)):
			scaling_factor=max([d[0][j] for d in data])
			if scaling_factor<1: scaling_factor=1
			for i in range(self.r.CAPTURE_CHUNKS):
				beatline=data[i][1]
				if not beatline:
					val=float(data[i][0][j])/scaling_factor
					pygame.draw.line(audio,colors[j],(i,(height-height*val)/2),(i,(height+height*val)/2))
				else:
					pygame.draw.line(audio,beatline_color,(i,0),(i,height))

		scaler=1.-self.pll.get_fractick()
		bg_color=(255,255,255)
		beat_c=[int(c*scaler) for c in bg_color]
		right=self.r.CAPTURE_CHUNKS-1
		pygame.draw.line(audio,beat_c,(right,0),(right,height))

		return audio

	def pattern_timeline_widget(self,height=100,width=400):
		timeline=pygame.Surface((width,height))
		seconds_per_pixel=float(self.r.CHUNK)/self.r.RATE
		seconds_per_beat=self.pll.period
		pixels_per_beat=seconds_per_beat/seconds_per_pixel
		now=self.pll.get_tick()
		occupied_space=[]
		patterns=self.seq.get_patterns()

		def overlaps(a,b,c,d):
			if b is None and d is None:
				return True
			if b is None:
				return d>a
			if d is None:
				return b>c
			return (c<a and a<d) or (c<b and b<d) or (a<c and c<b) or (a<d and d<b) or a == c or b == d

		for start,stop,p in patterns:
			slot=-1
			for i in range(len(occupied_space)):
				wontfit=False
				for o_start,o_stop in occupied_space[i]:
					#print "checking",(start,stop,o_start,o_stop)
					if overlaps(start,stop,o_start,o_stop):
						wontfit=True
						break
				if not wontfit:
					occupied_space[i].append((start,stop))
					slot=i
					break
			if slot<0:
				slot=len(occupied_space)
				occupied_space.append([(start,stop)])

			start_px=(start-now)*pixels_per_beat
			if stop is None:
				stop_px=width
			else:
				stop_px=(stop-now)*pixels_per_beat
			pygame.draw.rect(timeline,(50,50,50),((start_px,1+slot*20),(stop_px-start_px-2,18)))
			pygame.draw.rect(timeline,(255,255,255),((start_px,1+slot*20),(stop_px-start_px-2,18)),1)

		return timeline

	def tick(self,num):
		self.r.set_beatline()
		self.last_tick=time.time()

	def tempo_adjust(self,amt):
		tempo=round(self.pll.get_tempo(),0)
		tempo+=amt
		self.pll.set_tempo(tempo)

	def tap(self):
		t=time.time()
		if t-self.last_click>2:
			self.first_click=t
			self.clicks=0
		else:
			self.clicks+=1
			self.pll.sync_period((t-self.first_click)/self.clicks)
		self.last_click=t

	def phase(self):
		t=time.time()
		self.pll.sync_phase(t)

	def add_test_pattern(self):
		p=Pattern(4,[(0,'on'),(1,'off')])
		#self.seq.add_pattern(p,round(self.pll.get_tick(),0)+4)
		self.seq.add_pattern(p,round(self.pll.get_tick(),0)+4,round(self.pll.get_tick(),0)+8)

	def effect(self,coord,press):
		if press:
			print coord

Beat()
