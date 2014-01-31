import threading
import time

class TimeBase(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)
		self.daemon=True
		self.tickfn=[]
		self.next_time=time.time()
		self.last_time=0
		self.period=0.5
		self.beat=0

	def register_tickfn(self,tickfn):
		self.tickfn.append(tickfn)

	def stop(self):
		self.go=False

	def run(self):
		self.go=True
		while self.go:
			t=time.time()
			if t<self.next_time:
				time.sleep(self.next_time-t)
			else:
				self.beat+=1
				for fn in self.tickfn:
					fn(self.beat)
				self.last_time=self.next_time
				self.next_time+=self.period

	def get_fractick(self):
		t=time.time()
		denom=self.next_time-self.last_time
		if denom == 0:
			return 1.
		return max(min((t-self.last_time)/denom,1.),0.)

	def get_tick(self):
		return self.beat+self.get_fractick()

	def sync_phase(self,t):
		behind=self.next_time-t
		ahead=t-self.last_time
		if behind < ahead:
			self.next_time-=behind
		else:
			self.next_time+=ahead

	def sync_period(self,period):
		self.period=period

	def get_tempo(self):
		return 60./self.period

	def set_tempo(self,tempo):
		self.period=60./tempo

