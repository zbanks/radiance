beat-off
========

Beat matching and effects sequencer

Building
========

beat-off is built on SDL (1.2, not 2.0), Portaudio, PortMIDI, VAMP, and [flux](http://github.com/zbanks/flux).

Theoretically, you should need the following packages installed on your system (names from Ubuntu 15.04, but you get the idea...):
```
libsdl1.2-dev
libsdl-gfx1.2-dev
libsdl-ttf2.0-dev
libportaudio-dev
libportmidi-dev
vamp-plugin-sdk
libzmq-dev
```
...and you'll need to get [flux](http://github.com/zbanks/flux). Compiling this is described on github page.

However, at least in some versions of Ubuntu, `libportaudio` is out of date and you might need to build a newer version from source if you're having issues compiling.

Additionally, `libportmidi` is terrible. To make things worse, in Ubuntu/Debian it was [compiled with a debug option](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=765375) which prevents beat-off from catching errors. 

As a result, if your version of portmidi was compiled with this option, beat-off will crash if it encounters any MIDI errors, even if they would be recoverable. It's advised to recompile portmidi from source to prevent this.

Finally, although not required to build/run, you'll want the following VAMP plugins:

- [BTrack](https://github.com/adamstark/BTrack) - Causal beat detection
- [pYIN](https://code.soundsoftware.ac.uk/projects/pyin) - Pitch estimation
- [Queen Mary Set](http://isophonics.net/QMVampPlugins)

