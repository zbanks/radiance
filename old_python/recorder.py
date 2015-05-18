import threading
import pyaudio
import collections
import struct
import numpy
from math import *

class Recorder(threading.Thread):
	CHUNK = 512
	FORMAT = pyaudio.paInt16
	CHANNELS = 1
	RATE = 48000
	CHUNK_TIME = float(CHUNK)/RATE
	CAPTURE_CHUNKS = 512
	FRAME_DELAY = 10

	RANGES = [(50,200),(200,800),(800,2400)]

	def __init__(self,out=True):
		threading.Thread.__init__(self)
		self.daemon=True
		self.chunk_buffer=collections.deque()
		for i in range(self.CAPTURE_CHUNKS):	
			self.chunk_buffer.append(([0]*len(self.RANGES),False))

		self.pa = pyaudio.PyAudio()

		self.in_stream = self.pa.open(format=self.FORMAT,
			channels=self.CHANNELS,
			rate=self.RATE,
			input=True,
			frames_per_buffer=self.CHUNK)

		self.out=out
		if out:
			self.out_stream = self.pa.open(format=self.FORMAT,
				channels=self.CHANNELS,
				rate=self.RATE,
				output=True,
				frames_per_buffer=self.CHUNK)

			self.out_buf=collections.deque()
			self.default_out=struct.pack('h',0)
			self.default_out_buf=self.default_out*self.CHUNK

		print "LATENCY",1000*self.CHUNK_TIME,"ms"
		print "CAPTURE_CHUNKS",self.CAPTURE_CHUNKS

	def stop(self):
		self.go=False

	def run2(self):
		while self.go:
			if len(self.out_buf)>0:
				buf=self.out_buf.popleft()
			else:
				buf=self.default_out_buf
			self.out_stream.write(buf)

	def run(self):
		self.go=True
		self.beatline=False

		if self.out:
			self.playback=threading.Thread(target=self.run2)
			self.playback.daemon=True
			self.playback.start()

		while self.go:
			data = self.in_stream.read(self.CHUNK)
			samples = self.unpack(data)
			fft=numpy.fft.rfft(samples)
			fft=abs(fft)**2
			out=[]
			for (low,high) in self.RANGES:
				low_bucket=-int(self.RATE/2./low)
				high_bucket=-int(self.RATE/2./high)
				result=sum(fft[low_bucket:high_bucket])
				out.append(result)
			bl=False
			if self.beatline>0:
				self.beatline-=1
				if self.beatline==0:
					bl=True
			self.chunk_buffer.append((out,bl))
			self.chunk_buffer.popleft()

		self.playback.join()
		self.over()

	def tick(self):
		freq=880
		duration=0.02
		amp=16383
		tick=[amp*sin(i*2*pi*freq/self.RATE) for i in range(int(duration*self.RATE))]
		while len(tick)>0:
			if len(tick)<=self.CHUNK:
				outb=struct.pack('h'*len(tick),*tick)+self.default_out*(self.CHUNK-len(tick))
				tick=[]
			else:
				outb=struct.pack('h'*self.CHUNK,*tick[0:self.CHUNK])
				tick=tick[self.CHUNK:]
			self.out_buf.append(outb)

	def get_chunks(self):
		return list(self.chunk_buffer)

	def set_beatline(self):
		self.beatline=self.FRAME_DELAY
		self.tick()

	def over(self):
		self.in_stream.stop_stream()
		self.out_stream.stop_stream()
		self.in_stream.close()
		self.out_stream.close()
		self.pa.terminate()

	def unpack(self,data):
		return struct.unpack('h'*(len(data)/2),data)
