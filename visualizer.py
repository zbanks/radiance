#!/usr/bin/python

import numpy
import pyaudio
import time
import struct
import collections
from devices import SingleBespeckleDevice
import colorsys
import threading
import traceback
import math
import sys
from pygame import midi

CMD_SYNC = 0x80
CMD_TICK = 0x88
CMD_RESET = 0x83
CMD_REBOOT = 0x83
CMD_MSG = 0x81
CMD_STOP = 0x82
CMD_PARAM = 0x85

class Visualizer(threading.Thread):
    STRIP_LENGTH=50
    CHUNK = 1024
    FORMAT = pyaudio.paInt16
    CHANNELS = 1
    RATE = 48000
    HISTORY_SIZE = 500

    RANGES = [(20,200),(200,1200),(1200,2400),(2400,1200)]
    TAU_LPF = .1
    COLOR_PERIOD = 60


    def __init__(self):
        self.b = []
        self.enumerate_devices()
        self.init_audio()
        self.reset_strip()
        self.history=collections.deque()
        self.lpf_audio=[0]*len(self.RANGES)
        self.dt=float(self.CHUNK)/self.RATE
        self.alpha=self.dt/(self.TAU_LPF+self.dt)
        self.hue=0.
        self.stepfns = [self.step]
        self.smooth_dict = {}
        self.mute_delta = 0
        self.mute = 1.0
        self.whiteout_delta = 0
        self.whiteout = 1.0
        midi.init()
        self.mi = None
        try:
            self.mi = midi.Input(3)
        except:
            traceback.print_exc()
            print "Can't find midi, oh well"
        self.mstat = {}
        self.hueoff = 0
        super(Visualizer, self).__init__()

    def stop(self):
        self.stopped = True

    def restart(self):
        self.stopped = False

    def read_midi_events(self):
        if not self.mi:
            return
        for ev in self.mi.read(100):
            if len(ev[0]) == 4 and ev[0][0] == 176:
                self.mstat[ev[0][1]] = ev[0][2]

    def run(self):
        self.stopped = False
        while True:
            if self.stopped:
                time.sleep(0.05)
            else:
                # Tolerate errors in step()
                try: 
                    self.stepfns[-1]()
                except:
                    print "Error running fn step()"
                    traceback.print_exc()
                    if len(self.stepfns) > 1:
                        self.stepfns.pop()
                    else:
                        print "Failed to step! Fix your shit!"
                        self.stopped = True

    def unsaturate(self):
        h = self.history[-1]
        self.history = collections.deque()
        for i in range(self.HISTORY_SIZE):
            self.history.append(h)


    def inp(self, name, default=0):
        if name in self.mstat:
            return self.mstat[name]
        return default

    def step(self):
        def maxat(a): return max(enumerate(a), key=lambda x: x[1])[0] 
        self.read_midi_events()

        audio, fft = self.analyze_audio()
        mind = self.inp(24, 4)
        dom_freq = maxat(fft[mind:-mind]) + mind
        #self.dom_freq = dom_freq

        dom_freq = self.smooth("dom_freq", dom_freq, self.inp(14, 10) / 500.0)
        #sys.stdout.write("\rdom_freq %d" % dom_freq)

        octave = 2.0 ** math.floor(math.log(dom_freq) / math.log(2))
        self.octave = octave
        self.hue = ((dom_freq - octave) / octave + self.hueoff)
        self.hue += self.inp(14, 0) / 127.0
        self.hue = self.hue % 1.0

        self.lpf_audio=[self.lpf(float(data),mem,self.alpha) for data,mem in zip(audio,self.lpf_audio)]

        self.history.append(self.lpf_audio)
        if len(self.history)>self.HISTORY_SIZE:
            self.history.popleft()

        scaling_factor=[max(max([d[j] for d in self.history]),1) for j in range(len(self.RANGES))]

        levels=[a/f for a,f in zip(self.lpf_audio,scaling_factor)]
        bass_val = max(min((levels[0]-0.1)/0.9,1.), 0.0)
        bass_val = self.smooth("bass_val", bass_val, 0.6)
        bass_hue = (self.hue + 0.95) % 1.0
        if bass_val < 0.1: # Switch colors for low bass values
            bass_hue = (bass_hue + 0.9) % 1.0
            bass_val *= 1.1

        bass_val = max(bass_val ** 0.9 , 0.1)

        bass_color=self.makecolor(colorsys.hsv_to_rgb(bass_hue,1. * self.whiteout,bass_val * self.mute))
        treble_color=self.makecolor(colorsys.hsv_to_rgb(self.hue,0.7 * self.whiteout,1. * self.mute))

        treble_size = (0.5-0.3*levels[1]) 
        treble_size = self.smooth("treble_size", treble_size, 0.3)

        self.mute += self.mute_delta
        self.mute = max(min(self.mute, 1.0), 0.0)
        self.whiteout += self.whiteout_delta
        #self.whiteout = max(min(self.whiteout, 1.0), 0.0)
        self.mute = self.inp(11, 0) / 127.0


        self.update_strip([
            (0x10,0x00)+bass_color+(0xFF,),
            (0x05,0x01)+treble_color+(0xFF,self.makebyte(self.STRIP_LENGTH*treble_size),self.makebyte(self.STRIP_LENGTH*(0.5+0.3*levels[1]))),
            #(0x05,0x02,0x00,0xFF,0xFF,0xFF,self.makebyte(self.STRIP_LENGTH*(0.5-0.2*levels[2])),self.makebyte(self.STRIP_LENGTH*(0.5+0.2*levels[2]))),
        ])
        #print self.analyze_audio()

    def fade_out(self, tau=0.05):
        self.mute_delta = -tau
    def fade_in(self, tau=0.1):
        self.unsaturate()
        self.mute_delta = tau
    def white_out(self, tau=0.01):
        self.unsaturate()
        self.whiteout_delta = -tau
    def white_in(self, tau=0.01):
        self.whiteout_delta = tau

    def makecolor(self,c):
        return (int(c[0]*255),int(c[1]*127),int(c[2]*127))

    def makebyte(self,num):
        return int(max(min(num,255),0))

    def free_devices(self):
        for b in self.b:
            b.close()
        self.b = []

    def smooth(self, key, now, alpha=0.1, fn=None):
        if not fn:
            fn = self.lpf
        if key not in self.smooth_dict:
            self.smooth_dict[key] = now
        output = fn(now, self.smooth_dict[key], alpha)
        self.smooth_dict[key]  = output
        return output

    def enumerate_devices(self):
        self.free_devices()
        self.b = []
        for i in range(10):
            try:
                self.b.append(SingleBespeckleDevice('/dev/ttyUSB%d' % i, 3000000))
                if len(b) >= 4:
                    break
            except:
                pass

        print "Enumerated {0} devices.".format(len(self.b))

    def init_audio(self):
        #pyaudio.pa.initialize(pyaudio.pa)
        self.pa=pyaudio.PyAudio()

        #devs=[self.pa.get_device_info_by_index(i) for i in range(self.pa.get_device_count())]

        self.in_stream = self.pa.open(
            format=self.FORMAT,
            channels=self.CHANNELS,
            rate=self.RATE,
            input=True,
            frames_per_buffer=self.CHUNK)

    def analyze_audio(self):
        data = self.in_stream.read(self.CHUNK)
        samples = struct.unpack('h'*self.CHUNK,data)
        fft=numpy.fft.rfft(samples)
        fft=abs(fft)**2
        out=[]
        for (low,high) in self.RANGES:
            low_bucket=int(self.CHUNK*low/self.RATE)
            high_bucket=int(self.CHUNK*high/self.RATE)
            result=sum(fft[low_bucket:high_bucket])
            out.append(result)
        return out, fft

    def reset_strip(self):
        for d in self.b:
            d.framed_packet((CMD_RESET,))

    def update_strip(self,packets):
        for p in packets:
            for d in self.b:
                d.framed_packet(p)

        for d in self.b:
            d.framed_packet((CMD_SYNC,0))
        for d in self.b:
            d.flush()

    def diode_lpf(self,data,mem,alpha):
        if data>mem:
            return data
        return mem+alpha*(data-mem)

    def lpf(self,data,mem,alpha):
        return mem+alpha*(data-mem)

def setup():
    vis = Visualizer()
    vis.start()
    return vis


def refresh(mod, vis):
    try: 
        reload(mod)
    except:
        traceback.print_exc()
        print "Didn't reload"
        return vis

    for key, item in mod.Visualizer.__dict__.items():
        if key not in ("run", "stepfns"):
            if hasattr(item, '__call__'): # need to re-bind methods first
                vis.__dict__[key] = item.__get__(vis, mod.Visualizer)
            else:
                vis.__dict__[key] = item
    vis.stepfns.append(vis.step)

    vis.restart()
    return vis

"""
import visualizer
v = visualizer.setup()
visualizer.refresh(visualizer, v)
"""

if __name__ == "__main__":
    setup()
