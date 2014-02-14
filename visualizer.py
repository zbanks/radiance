#!/usr/bin/python

import numpy
import pyaudio
import time
import struct
import collections
from devices import SingleBespeckleDevice
import colorsys

CMD_SYNC = 0x80
CMD_TICK = 0x88
CMD_RESET = 0x83
CMD_REBOOT = 0x83
CMD_MSG = 0x81
CMD_STOP = 0x82
CMD_PARAM = 0x85

class Visualizer:
	STRIP_LENGTH=50
	CHUNK = 1024
	FORMAT = pyaudio.paInt16
	CHANNELS = 1
	RATE = 48000
	HISTORY_SIZE = 500

	RANGES = [(50,200),(200,800),(800,2400),(2400,1200)]
	TAU_LPF = .1
	COLOR_PERIOD = 60

	def __init__(self):
		self.enumerate_devices()
		self.init_audio()
		self.reset_strip()
		self.history=collections.deque()
		self.lpf_audio=[0]*len(self.RANGES)
		dt=float(self.CHUNK)/self.RATE
		alpha=dt/(self.TAU_LPF+dt)
		hue=0.
		while True:
			hue=(hue+dt/self.COLOR_PERIOD)%1.
			audio=self.analyze_audio()
			self.lpf_audio=[self.lpf(float(data),mem,alpha) for data,mem in zip(audio,self.lpf_audio)]

			self.history.append(self.lpf_audio)
			if len(self.history)>self.HISTORY_SIZE:
				self.history.popleft()

			scaling_factor=[max(max([d[j] for d in self.history]),1) for j in range(len(self.RANGES))]

			levels=[a/f for a,f in zip(self.lpf_audio,scaling_factor)]

			bass_color=self.makecolor(colorsys.hsv_to_rgb(hue,1.,max(min((levels[0]-0.1)/0.9,1.),0.)))
			treble_color=self.makecolor(colorsys.hsv_to_rgb(hue,0.7,1.))

			self.update_strip([
				(0x10,0x00)+bass_color+(0xFF,),
				(0x05,0x01)+treble_color+(0xFF,self.makebyte(self.STRIP_LENGTH*(0.5-0.3*levels[1])),self.makebyte(self.STRIP_LENGTH*(0.5+0.3*levels[1]))),
				#(0x05,0x02,0x00,0xFF,0xFF,0xFF,self.makebyte(self.STRIP_LENGTH*(0.5-0.2*levels[2])),self.makebyte(self.STRIP_LENGTH*(0.5+0.2*levels[2]))),
			])
			#print self.analyze_audio()

	def makecolor(self,c):
		return (int(c[0]*255),int(c[1]*127),int(c[2]*127))

	def makebyte(self,num):
		return int(max(min(num,255),0))

	def enumerate_devices(self):
		self.b = []
		for i in range(10):
			try:
				self.b.append(SingleBespeckleDevice('/dev/ttyUSB%d' % i, 3000000))
				if len(b) >= 4:
					break
			except:
				pass

		print "Enumerated {0} devices.".format(len(self.b))

	def init_audio(self):
		#pyaudio.pa.initialize(pyaudio.pa)
		self.pa=pyaudio.PyAudio()

		#devs=[self.pa.get_device_info_by_index(i) for i in range(self.pa.get_device_count())]

		self.in_stream = self.pa.open(
			format=self.FORMAT,
			channels=self.CHANNELS,
			rate=self.RATE,
			input=True,
			frames_per_buffer=self.CHUNK)

	def analyze_audio(self):
		data = self.in_stream.read(self.CHUNK)
		samples = struct.unpack('h'*self.CHUNK,data)
		fft=numpy.fft.rfft(samples)
		fft=abs(fft)**2
		out=[]
		for (low,high) in self.RANGES:
			low_bucket=int(self.CHUNK*low/self.RATE)
			high_bucket=int(self.CHUNK*high/self.RATE)
			result=sum(fft[low_bucket:high_bucket])
			out.append(result)
		return out

	def reset_strip(self):
		for d in self.b:
			d.framed_packet((CMD_RESET,))

	def update_strip(self,packets):
		for p in packets:
			for d in self.b:
				d.framed_packet(p)

		for d in self.b:
			d.framed_packet((CMD_SYNC,0))
		for d in self.b:
			d.flush()

	def diode_lpf(self,data,mem,alpha):
		if data>mem:
			return data
		return mem+alpha*(data-mem)

	def lpf(self,data,mem,alpha):
		return mem+alpha*(data-mem)

Visualizer()
