class Effect:
	def __init__(self,cmd,data,persistent=True,render=None):
		self.cmd=cmd
		self.data=data
		self.persistent=persistent
		self.render=render
