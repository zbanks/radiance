import threading
import pyaudio
import collections
import struct
import numpy

class Recorder(threading.Thread):
	CHUNK = 512
	FORMAT = pyaudio.paInt16
	CHANNELS = 1
	RATE = 48000
	CHUNK_TIME = float(CHUNK)/RATE
	CAPTURE_CHUNKS = 512
	FRAME_DELAY = 3

	RANGES = [(50,200),(200,800),(800,2400)]

	def __init__(self):
		threading.Thread.__init__(self)
		self.daemon=True
		self.chunk_buffer=collections.deque()
		for i in range(self.CAPTURE_CHUNKS):	
			self.chunk_buffer.append(([0]*len(self.RANGES),False))

		self.pa = pyaudio.PyAudio()

		self.stream = self.pa.open(format=self.FORMAT,
			channels=self.CHANNELS,
			rate=self.RATE,
			input=True,
			frames_per_buffer=self.CHUNK)

		print "LATENCY",1000*self.CHUNK_TIME,"ms"
		print "CAPTURE_CHUNKS",self.CAPTURE_CHUNKS

	def stop(self):
		self.go=False

	def run(self):
		self.go=True
		self.beatline=False
		while self.go:
			data = self.stream.read(self.CHUNK)
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

		self.over()

	def get_chunks(self):
		return list(self.chunk_buffer)

	def set_beatline(self):
		self.beatline=self.FRAME_DELAY

	def over(self):
		self.stream.stop_stream()
		self.stream.close()
		self.pa.terminate()

	def unpack(self,data):
		return struct.unpack('h'*(len(data)/2),data)
