#!/usr/bin/python

import pygame
from recorder import Recorder
from timebase import TimeBase
from seq import Sequencer
from bespeckle import OutputAdapter
from devices import SingleBespeckleDevice,FakeSingleBespeckleDevice
from pattern import Pattern
from effect import Effect
import time
import numpy
import scipy.signal
import collections
import sys

class Beat:

	KEY_TAP=pygame.K_SPACE
	KEY_PHASE=pygame.K_RETURN

	EFFECT_HISTORY_LENGTH=2

	KEYBOARD={
		pygame.K_q:(0,0),
		pygame.K_w:(1,0),
		pygame.K_e:(2,0),
		pygame.K_r:(3,0),
		pygame.K_t:(4,0),
		pygame.K_y:(5,0),
		pygame.K_u:(6,0),
		pygame.K_i:(7,0),
		pygame.K_a:(0,1),
		pygame.K_s:(1,1),
		pygame.K_d:(2,1),
		pygame.K_f:(3,1),
		pygame.K_g:(4,1),
		pygame.K_h:(5,1),
		pygame.K_j:(6,1),
		pygame.K_k:(7,1),
		pygame.K_z:(0,2),
		pygame.K_x:(1,2),
		pygame.K_c:(2,2),
		pygame.K_v:(3,2),
		pygame.K_b:(4,2),
		pygame.K_n:(5,2),
		pygame.K_m:(6,2),
		pygame.K_COMMA:(7,2),
	}

	FUNCTION_KEYS={
		pygame.K_F1:1,
		pygame.K_F2:2,
		pygame.K_F3:3,
		pygame.K_F4:4,
		pygame.K_F5:5,
		pygame.K_F6:6,
		pygame.K_F7:7,
		pygame.K_F8:8,
		pygame.K_F9:9,
		pygame.K_F10:10,
		pygame.K_F11:11,
		pygame.K_F12:12,
	}

	PATTERN_KEYS={
		pygame.K_1:1,
		pygame.K_2:2,
		pygame.K_3:3,
		pygame.K_4:4,
		pygame.K_5:5,
		pygame.K_6:6,
		pygame.K_7:7,
		pygame.K_8:8,
		pygame.K_9:9,
		pygame.K_0:0,
	}

	KB_REMAP={
		pygame.K_RIGHTBRACKET:(0,1),
		pygame.K_QUOTE:(1,1),
		pygame.K_SLASH:(2,1),
		pygame.K_LEFTBRACKET:(0,-1),
		pygame.K_SEMICOLON:(1,-1),
		pygame.K_PERIOD:(2,-1),
	}

	COLORS=[
		(255,255,255), # white
		(255,0,0), # red
		(255,20,0), # orange
		(255,80,0), # yellow
		(50,100,0), # green
		(0,30,80), # blue
		(255,0,50), # purple
		(0,0,0), # black
	]

	EFFECT_NAMES=[
		'on',
		'strobe',
		'sweep',
		'rev sweep',
		'fast sweep',
		'fast rev sweep',
		'pulse',
		'rev pulse',
		'fast pulse',
		'fast rev pulse',
		'fade in',
		'fade out',
		'fast fade in',
		'fast fade out',
	]

	def __init__(self):
		pygame.init()
		self.size=1024,480
		self.screen=pygame.display.set_mode(self.size)
		self.bg=0,0,0
		self.fg=255,255,255
		self.audio_color=0,0,255

		self.first_click=0
		self.last_click=0
		self.clicks=0
		self.last_tick=0

		self.effects={}
		self.function=1
		self.shift=False
		self.forever=False
		self.forever_patterns={}
		self.forever_fn={}
		self.last_stop=0
		self.recording=False
		self.recorded={}
		self.coarse_grain=1
		self.fine_grain=4
		self.saved_patterns={}
		self.rec_pat=None
		self.kb_map=[0,1,2]
		self.last_effect=collections.deque()

		self.r=Recorder()
		self.tb=TimeBase()
		self.tb.register_tickfn(self.tick)
		if len(sys.argv)>=2 and sys.argv[1]=='fake':
			b=FakeSingleBespeckleDevice('/dev/ttyUSB0',115200)
		else:
			b=SingleBespeckleDevice('/dev/ttyUSB0',115200)

		self.oa=OutputAdapter(b,self.tb)
		self.oa.add_reset()

		self.seq=Sequencer(self.tb,self.oa)

		self.oa.start()
		self.r.start()
		self.tb.start()
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
						self.effect(self.KEYBOARD[event.key])
					elif event.key==pygame.K_TAB:
						self.start_recording()
					elif event.key in self.PATTERN_KEYS:
						k=self.PATTERN_KEYS[event.key]
						if self.rec_pat is not None:
							self.save_pattern(k)
						else:
							p=self.pattern_map(k)
							if self.forever:
								if k in self.forever_patterns:
									self.remove_pattern(self.forever_patterns[k])
									self.forever_patterns.pop(k)
								elif p is not None:
									self.forever_patterns[k]=self.add_pattern(p,self.shift,self.forever)
							elif p is not None:
								self.add_pattern(p,self.shift,self.forever)
					elif event.key in self.FUNCTION_KEYS:
						k=self.FUNCTION_KEYS[event.key]
						p=self.function_map(k)
						if self.forever:
							if k in self.forever_fn:
								self.remove_pattern(self.forever_fn[k])
								self.forever_fn.pop(k)
							elif p is not None:
								self.forever_fn[k]=self.add_pattern(p,self.shift,self.forever)
						elif p is not None:
							self.add_pattern(p,self.shift,self.forever)
					elif event.key in self.KB_REMAP:
						self.kb_remap(*self.KB_REMAP[event.key])
					elif event.key==pygame.K_LSHIFT:
						self.shift=True
					elif event.key==pygame.K_LCTRL:
						self.forever=True
				elif event.type==pygame.KEYUP:
					if event.key in self.KEYBOARD:
						self.effect_release(self.KEYBOARD[event.key])
					elif event.key==pygame.K_LSHIFT:
						self.shift=False
					elif event.key==pygame.K_LCTRL:
						self.forever=False
					elif event.key==pygame.K_TAB:
						self.stop_recording()

			if quit:
				break

			font = pygame.font.Font(None, 36)
			text = font.render("TEMPO:"+str(self.tb.get_tempo())+" BPM", 1, self.fg)
			if self.recording:
				msg='recording'
			elif self.rec_pat is not None:
				msg='press a key to store'
			else:
				msg='not recording'
			text2 = font.render(msg,1,self.fg)

			audio=self.audio_widget()
			pattern_timeline=self.pattern_timeline_widget()

			self.screen.fill(self.bg)
			self.screen.blit(text, (0,0))
			self.screen.blit(text2, (0,30))
			self.screen.blit(audio, (0,60))
			self.screen.blit(pattern_timeline, (audio.get_width(),60))

			kb=[font.render(self.EFFECT_NAMES[self.kb_map[row]],1,self.fg) for row in range(3)]
			self.screen.blit(kb[0], (0,200))
			self.screen.blit(kb[1], (0,230))
			self.screen.blit(kb[2], (0,260))

			pygame.display.flip()
			time.sleep(0)

		self.seq.stop()
		self.r.stop()
		self.tb.stop()
		self.oa.stop()
		self.seq.join()
		self.r.join()
		self.tb.join()
		self.oa.join()

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

		scaler=1.-self.tb.get_fractick()
		bg_color=(255,255,255)
		beat_c=[int(c*scaler) for c in bg_color]
		right=self.r.CAPTURE_CHUNKS-1
		pygame.draw.line(audio,beat_c,(right,0),(right,height))

		return audio

	def pattern_timeline_widget(self,height=100,width=400):
		timeline=pygame.Surface((width,height))
		seconds_per_pixel=float(self.r.CHUNK)/self.r.RATE
		seconds_per_beat=self.tb.period
		pixels_per_beat=seconds_per_beat/seconds_per_pixel
		now=self.tb.get_tick()
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

		def render_pattern(surface,pat,start,start_px,stop_px,slot):
			pygame.draw.rect(surface,(50,50,50),((start_px,1+slot*20),(stop_px-start_px-2,18)))

			for start,duration,effect in pat.get_events((start_px-(start-now)*pixels_per_beat)/pixels_per_beat,(stop_px-(start-now)*pixels_per_beat)/pixels_per_beat):
				if effect.render is None:
					continue
				eff_start_px=start_px+start*pixels_per_beat
				eff_stop_px=eff_start_px+duration*pixels_per_beat
				if eff_stop_px <= start_px or eff_start_px >= stop_px:
					continue
				effect.render(surface,eff_start_px,eff_stop_px,start_px,stop_px,slot)

			font = pygame.font.Font(None, 16)
			text = font.render(pat.name, 1, (255,255,255))
			x=min(max(5,start_px+5),stop_px-text.get_width()-8)
			surface.blit(text,(x,2+slot*20))

			pygame.draw.rect(surface,(255,255,255),((start_px,1+slot*20),(stop_px-start_px-2,18)),1)

		for start,stop,p in patterns:
			slot=-1
			for i in range(len(occupied_space)):
				wontfit=False
				for o_start,o_stop in occupied_space[i]:
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

			render_pattern(timeline,p,start,start_px,stop_px,slot)

		return timeline

	def full_color(self,surface,eff_start_px,eff_stop_px,start_px,stop_px,slot):
		start=max(eff_start_px,start_px)
		stop=min(eff_stop_px,stop_px)
		pygame.draw.rect(surface,(0,0,50),((start,1+slot*20),(stop-start-2,18)))

	def tick(self,num):
		self.r.set_beatline()
		self.last_tick=time.time()
		self.oa.add_tick()

	def tempo_adjust(self,amt):
		tempo=round(self.tb.get_tempo(),0)
		tempo+=amt
		self.tb.set_tempo(tempo)

	def tap(self):
		t=time.time()
		if t-self.last_click>2:
			self.first_click=t
			self.clicks=0
		else:
			self.clicks+=1
			self.tb.sync_period((t-self.first_click)/self.clicks)
		self.last_click=t

	def phase(self):
		t=time.time()
		self.tb.sync_phase(t)

	#def add_test_pattern(self):
	#	eff=0x10,self.effect_color+(0xFF,)
	#	p=Pattern(self.oa,4,[(0,1,eff)])
	#	self.seq.add_pattern(p,round(self.pll.get_tick(),0)+4)
	#	self.seq.add_pattern(p,round(self.pll.get_tick(),0)+4,round(self.pll.get_tick(),0)+8)

	def kb_remap(self,remap_row,increment):
		self.kb_map[remap_row]=(self.kb_map[remap_row]+increment)%len(self.EFFECT_NAMES)

	def effect_map(self,(c,r)):
		if c>=len(self.COLORS):
			return None
		en=self.EFFECT_NAMES[self.kb_map[r]]
		opacity=0xFF
		if self.shift:
			opacity/=2

		color=self.COLORS[c]+(opacity,)
		if en=='on':
			return Effect(0x10,color,render=self.full_color)
		if en=='strobe':
			return Effect(0x40,color+(0x00,0x10),False)
		if en=='sweep':
			return Effect(0x41,color+(0x02,0x00))
		if en=='rev sweep':
			return Effect(0x41,color+(0x82,0x00))
		if en=='pulse':
			return Effect(0x42,color+(0x02,0x00),False)
		if en=='rev pulse':
			return Effect(0x42,color+(0x82,0x00),False)
		if en=='fast sweep':
			return Effect(0x41,color+(0x01,0x00))
		if en=='fast rev sweep':
			return Effect(0x41,color+(0x81,0x00))
		if en=='fast pulse':
			return Effect(0x42,color+(0x01,0x00),False)
		if en=='fast rev pulse':
			return Effect(0x42,color+(0x081,0x00),False)
		if en=='fade in':
			return Effect(0x43,color+(0x02,0x00))
		if en=='fade out':
			return Effect(0x43,color+(0x82,0x00))
		if en=='fast fade in':
			return Effect(0x43,color+(0x01,0x00))
		if en=='fast fade out':
			return Effect(0x43,color+(0x81,0x00))
		return None

	def pattern_map(self,p_num):
		p=None
		if p_num in self.saved_patterns:
			p=self.saved_patterns[p_num]
		if p is None:
			return None
		p_name='{0}'.format(p_num)
		return p+(p_name,)

	def function_map(self,f_num):
		if len(self.last_effect)<self.EFFECT_HISTORY_LENGTH:
			return None
		e1=self.last_effect[1]
		e2=self.last_effect[0]
		p=None
		if f_num==1:
			p=(4,[(0,4,e1),(1,4,e2),(2,4,e1),(3,4,e2)])
		elif f_num==2:
			p=(4,[(0,4,e1),(2,4,e2)])
		elif f_num==3:
			p=(8,[(0,8,e1),(4,8,e2)])
		elif f_num==4:
			p=(4,[(0,0.5,e1),(0.5,0.5,e2),(1,0.5,e1),(1.5,0.5,e2),(2,0.5,e1),(2.5,0.5,e2),(3,0.5,e1),(3.5,0.5,e2)])
		elif f_num==5:
			p=(4,[(1,4,e1),(3,4,e2)])
		if p is None:
			return None
		p_name='F{0}'.format(f_num)
		return p+(p_name,)

	def save_pattern(self,p_num):
		self.saved_patterns[p_num]=self.rec_pat
		self.rec_pat=None

	def effect(self,coord):
		eff=self.effect_map(coord)
		self.last_effect.appendleft(eff)
		if len(self.last_effect)>self.EFFECT_HISTORY_LENGTH:
			self.last_effect.pop()
		if eff is None:
			return
		if self.recording:
			cur_time=self.align(self.tb.get_tick(),self.fine_grain)
			#cur_time=self.tb.get_tick()
			start=cur_time-self.recording_start
			self.recorded[coord]=(start,eff)

		if eff.persistent:
			self.effects[coord]=self.oa.add_persistent_effect(eff.cmd,eff.data)
		else:
			self.oa.add_transient_effect(eff.cmd,eff.data)
			self.effects[coord]=None

	def effect_release(self,coord):
		if coord in self.effects:
			if self.effects[coord] is not None:
				self.oa.remove_effect(self.effects[coord])
			self.effects.pop(coord)
		if coord in self.recorded:
			cur_time=self.align(self.tb.get_tick(),self.fine_grain)
			#cur_time=self.tb.get_tick()
			stop=cur_time-self.recording_start
			start,eff=self.recorded[coord]
			self.record.append((start,stop-start,eff))
			self.recorded.pop(coord)

	def align(self,loc,grain):
		return round(loc*grain,0)/grain

	def start_recording(self):
		self.record=[]
		self.recording=True
		self.recorded={}
		self.recording_start=self.align(self.tb.get_tick(),self.coarse_grain)
		for coord in self.effects:
			eff=self.effect_map(coord)
			self.recorded[coord]=(0,eff)

	def stop_recording(self):
		stop=round(self.tb.get_tick()*self.coarse_grain,0)/self.coarse_grain
		duration=stop-self.recording_start

 		for start,eff in self.recorded.values():
			self.record.append((start,stop-start,eff))
		self.recorded={}
		self.recording=False
		self.rec_pat=(duration,self.record)
		self.record=[]
		print self.rec_pat

	def remove_pattern(self,p_id):
		self.seq.remove_pattern(p_id)

	def add_pattern(self,p,stack,forever):
		start=round(self.tb.get_tick()*self.coarse_grain,0)/self.coarse_grain
		if stack and self.last_stop>start:
			start=self.last_stop

		l=p[0]
		if forever:
			return self.seq.add_pattern(Pattern(self.oa,*p),start,None)
		self.last_stop=start+l
		return self.seq.add_pattern(Pattern(self.oa,*p),start,start+l)


Beat()
