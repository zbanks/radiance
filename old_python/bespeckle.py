import devices
import Queue
import threading
import time

CMD_SYNC = 0x80
CMD_TICK = 0x88
CMD_RESET = 0x83
CMD_REBOOT = 0x83
CMD_MSG = 0x81
CMD_STOP = 0x82
CMD_PARAM = 0x85

class OutputAdapter(threading.Thread):
	def __init__(self,b,timebase):
		threading.Thread.__init__(self)
		self.daemon=True
		self.b=b
		self.timebase=timebase
		self.q=Queue.Queue()
		self.id_allocation=set()
		self.cur_id=0

	def get_id(self):
		self.cur_id=(self.cur_id+1)%256
		my_id=self.cur_id
		while self.cur_id in self.id_allocation:
			my_id=(my_id+1)%256
		return my_id

	def add_persistent_effect(self,cmd,data):
		my_id=self.get_id()
		self.id_allocation.add(my_id)
		self.q.put([cmd,my_id]+list(data))
		return my_id

	def add_transient_effect(self,cmd,data):
		my_id=self.get_id()
		self.q.put([cmd,my_id]+list(data))

	def add_message(self,to,data):
		self.q.put([CMD_MSG,to]+list(data))

	def remove_effect(self,eff_id):
		self.q.put([CMD_STOP,eff_id])
		self.id_allocation.remove(eff_id)

	def add_reset(self):
		self.q.put([CMD_RESET])

	def add_tick(self):
		self.q.put([CMD_TICK])

	def stop(self):
		self.go=False

	def run(self):
		self.go=True
		while self.go:
			while not self.q.empty():
				data=self.q.get()
				print ' '.join([hex(d) for d in data])
				for d in self.b:
					d.framed_packet(data)
			cur_time=self.timebase.get_tick()
			fractick=int(240*cur_time)%240
			for d in self.b:
				d.framed_packet([CMD_SYNC,fractick])
			for d in self.b:
				d.flush()
			time.sleep(0)
