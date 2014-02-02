#!/usr/bin/python

import pygame
from recorder import Recorder
from timebase import TimeBase
from seq import Sequencer
from bespeckle import OutputAdapter
from devices import SingleBespeckleDevice,FakeSingleBespeckleDevice
from pattern import Pattern
from effect import Effect
import graphics
import time
import numpy
import scipy.signal
import collections
import sys

class Beat:

	KEY_TAP=pygame.K_SPACE
	KEY_PHASE=pygame.K_RETURN
	KEY_RESET=pygame.K_ESCAPE

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
					elif event.key==self.KEY_RESET:
						self.oa.add_reset()
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

			audio=graphics.audio_widget(self.r,self.tb)
			pattern_timeline=graphics.pattern_timeline_widget(self.tb,self.seq,float(self.r.CHUNK)/self.r.RATE)

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
			return Effect(0x10,color,render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='strobe':
			return Effect(0x40,color+(0x00,0x10),False,render=lambda *x:graphics.strobe(graphics.strip2screen(color),*x))
		if en=='sweep':
			return Effect(0x41,color+(0x02,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='rev sweep':
			return Effect(0x41,color+(0x82,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='pulse':
			return Effect(0x42,color+(0x02,0x00),False,render=lambda *x:graphics.strobe(graphics.strip2screen(color),*x))
		if en=='rev pulse':
			return Effect(0x42,color+(0x82,0x00),False,render=lambda *x:graphics.strobe(graphics.strip2screen(color),*x))
		if en=='fast sweep':
			return Effect(0x41,color+(0x01,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='fast rev sweep':
			return Effect(0x41,color+(0x81,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='fast pulse':
			return Effect(0x42,color+(0x01,0x00),False,render=lambda *x:graphics.strobe(graphics.strip2screen(color),*x))
		if en=='fast rev pulse':
			return Effect(0x42,color+(0x081,0x00),False,render=lambda *x:graphics.strobe(graphics.strip2screen(color),*x))
		if en=='fade in':
			return Effect(0x43,color+(0x02,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='fade out':
			return Effect(0x43,color+(0x82,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='fast fade in':
			return Effect(0x43,color+(0x01,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
		if en=='fast fade out':
			return Effect(0x43,color+(0x81,0x00),render=lambda *x:graphics.full_color(graphics.strip2screen(color),*x))
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
