import threading
import time

class Sequencer(threading.Thread):
	def __init__(self,timebase,output_adapter):
		threading.Thread.__init__(self)
		self.daemon=True
		self.timebase=timebase
		self.output_adapter=output_adapter
		self.patterns={}
		self.pat_id=0
		self.lock=threading.Semaphore()

	def add_pattern(self,pattern,start,stop=None):
		my_id=self.get_next_id()
		self.patterns[my_id]=(start,stop,pattern)
		return my_id

	def get_next_id(self):
		self.pat_id+=1
		return self.pat_id

	def get_patterns(self):
		self.lock.acquire()
		v=self.patterns.values()
		self.lock.release()
		return self.patterns.values()

	def remove_pattern(self,pat_id):
		self.lock.acquire()
		self.remove_pattern_unsafe()
		self.lock.release()

	def remove_pattern_unsafe(self,pat_id):
		self.patterns[pat_id][2].remove()
		self.patterns.pop(pat_id)

	def stop(self):
		self.go=False

	def run(self):
		self.go=True
		while self.go:
			t=self.timebase.get_tick()
			self.lock.acquire()
			ids_to_remove=[pat_id for (pat_id,(start,stop,p)) in self.patterns.iteritems() if stop is not None and stop < t]
			for pat_id in ids_to_remove:
				self.remove_pattern_unsafe(pat_id)

			for start,stop,p in self.patterns.values():
				if start < t:
					p.process(t-start)
			self.lock.release()
			time.sleep(0)

class PrintingOutputAdapter:
	def __init__(self):
		pass

	def send(self,events):
		for e in events:
			print e

