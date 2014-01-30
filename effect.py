class Effect:
	def __init__(self,cmd,data,persistent=True):
		self.cmd=cmd
		self.data=data
		self.persistent=persistent
