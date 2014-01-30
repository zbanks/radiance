class Pattern:
	def __init__(self,oa,length=4,events=[],name='unnamed'):
		self.length=length
		self.events=events
		self.name=name
		self.last_tick=0.
		self.oa=oa
		self.active_events=[]

	def remove(self):
		for event_stop,res in self.active_events:
			self.oa.bespeckle_pop_effect(res)
		self.active_events=[]

	def process(self,tick):
		prev_iter=int(self.last_tick / self.length)
		cur_iter=int(tick / self.length)
		print tick

		for event_stop,res in self.active_events:
			if tick > event_stop:
				self.oa.bespeckle_pop_effect(res)
		self.active_events=[(event_stop,res) for event_stop,res in self.active_events if tick <= event_stop]

		for i in range(prev_iter,cur_iter+1):
			for timestamp,duration,event in self.events:
				event_start=timestamp+self.length*i
				event_stop=timestamp+self.length*i+duration
				if event_start >= self.last_tick and event_start < tick and event_stop > tick:
					res=self.oa.bespeckle_add_effect(*event)
					self.active_events.append((event_stop,res))
		self.last_tick=tick
