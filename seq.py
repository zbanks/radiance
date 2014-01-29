import threading
import time

class Sequencer(threading.Thread):
	def __init__(self,timebase,output_adapter):
		threading.Thread.__init__(self)
		self.daemon=True
		self.timebase=timebase
		self.output_adapter=output_adapter
		self.patterns=[]

	def add_pattern(self,pattern,start,stop=None):
		self.patterns.append((start,stop,pattern))

	def get_patterns(self):
		return self.patterns

	def stop(self):
		self.go=False

	def run(self):
		self.go=True
		while self.go:
			t=self.timebase.get_tick()
			for start,stop,p in self.patterns:
				if stop is not None and stop < t:
					self.output_adapter.send(p.remove())
			self.patterns=[(start,stop,p) for start,stop,p in self.patterns if stop is None or stop >= t]
			for start,stop,p in self.patterns:
				if start < t:
					self.output_adapter.send(p.process(t-start))
			time.sleep(0.01)

class PrintingOutputAdapter:
	def __init__(self):
		pass

	def send(self,events):
		for e in events:
			print e

