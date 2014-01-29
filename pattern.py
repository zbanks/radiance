class Pattern:
	def __init__(self,length=4,events=[],name='unnamed'):
		self.length=length
		self.events=events
		self.name=name
		self.last_tick=0.

	def remove(self):
		return []

	def process(self,tick):
		prev_iter=int(self.last_tick / self.length)
		cur_iter=int(tick / self.length)

		out=[]
		for i in range(prev_iter,cur_iter+1):
			for timestamp,event in self.events:
				event_time=timestamp+self.length*i
				if event_time >= self.last_tick and event_time < tick:
					out.append(event)
		self.last_tick=tick
		return out
