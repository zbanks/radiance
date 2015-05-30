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

Finally, although not required to build/run, you'll want the following VAMP plugins. You can install them to e.g. `~/vamp`.

- [BTrack](https://github.com/adamstark/BTrack) - Causal beat detection
- [pYIN](https://code.soundsoftware.ac.uk/projects/pyin) - Pitch estimation
- [Queen Mary Set](http://isophonics.net/QMVampPlugins)

Configuration
=============
Most of the configuation is done by editing `.h` files.

### UI
The UI is largely parametrically laid out, and is currently the largest component that can be modified without recompiling. The configuration file `layout.ini` can theoretically be used to "skin" the interface. 

However, currently beat-off just uses the default values defined in `ui/layout.def` and dumps these values to `layout.ini` when run.

### Output Devices (LED Strips)
Currently, beat-off only supports outputting to LED strips over flux. 

LED strips are laid out as a sequence of verticies placed on a [-1, 1]^2 square. The strip id's and these verticies can be configured in `output/slice.h`.

### MIDI Devices
beat-off is built around the Korg NanoKontrol 2 and NanoPad 2, but would work well with other (slider- and knob-heavy) controllers.

Although most of the MIDI controls are configured while the program runs, the name(s) of the MIDI device(s) are configured at compile time in `midi/controllers.c` and "permanent" bindings are built in `midi/layout.c`

`midi/controllers.h` defines each key/knob name for the NanoKontrol 2 and NanoPad 2, but this  isn't required to get a new device running. The key thing is for the `.name` attribute of the `controllers_enabled` struct to match your device.

### Signals
The signals visible in the UI are defined in the `signals` array in `signals/signal.c`.

### Filters
The filters visible in the UI are defined in the `filters` array in `filters/filter.c`. These correspond to VAMP plugins, which should be in your VAMP path.

### Default Patterns / Hits
The default patterns and hits in the slots are loaded in `core/main.c`.
