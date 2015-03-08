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
		self.lock=threading.Semaphore()

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
				self.lock.acquire()
				self.beat+=1
				self.last_time=self.next_time
				self.next_time+=self.period
				self.lock.release()
				for fn in self.tickfn:
					fn(self.beat)

	def get_fractick(self):
		self.lock.acquire()
		t=time.time()
		denom=self.next_time-self.last_time
		res=max(min((t-self.last_time)/denom,1.),0.)
		self.lock.release()
		return res

	def get_tick(self):
		self.lock.acquire()
		t=time.time()
		b=self.beat
		denom=self.next_time-self.last_time
		ft=max(min((t-self.last_time)/denom,1.),0.)
		self.lock.release()
		return b+ft

	def sync_phase(self,t):
		self.lock.acquire()
		behind=self.next_time-t
		ahead=t-self.last_time
		if behind < ahead:
			self.next_time-=behind
		else:
			self.next_time+=ahead
		self.lock.release()

	def sync_period(self,period):
		self.lock.acquire()
		self.period=period
		self.lock.release()

	def get_tempo(self):
		self.lock.acquire()
		tempo=60./self.period
		self.lock.release()
		return tempo

	def set_tempo(self,tempo):
		self.lock.acquire()
		self.period=60./tempo
		self.lock.release()

