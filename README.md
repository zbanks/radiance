Radiance
========

LJ software for controlling light displays. Supports beat detection, MIDI controllers, and output over [lux](http://github.com/ervanalb/lux).

Please [submit](http://github.com/zbanks/radiance/issues) any feature requests or issues you have. Especially let us know if you have trouble getting it to compile.

Screenshots
-----------

![screenshot](https://raw.githubusercontent.com/zbanks/radiance/master/radiance-screenshot.png)

How it works
------------
*(Todo: This should be fleshed out a lot more)*

### Timebase 
The *timebase* is a global relative time that is synced to the beat of the incoming audio. 

Internally, each beat is subdivided into 1000 *"millibeats"*. The timebase is guaranteed to be monotonic, but it can pause or change speed to stay in sync with the beat. The beats 'hit' on multiples of 1000 millibeats, e.g. *0, 1000, 2000, ...*

All of the *patterns*, *hits*, and *signals* use times from the timebase to sync to the ambient audio.

### Patterns
*Patterns* are visual effects on a plane that continuously change over time.

Each pattern and hit also has the following things:

#### Alpha
*Alpha* represents opacity, with 1 corresponding to opaque and 0 corresponding to invisible/transparent. If a pattern's alpha is set to 0, then it produces no output. The preview for each pattern does not factor in the alpha setting.

Alpha can be used to crossfade between patterns, similar to volume in an audio setup.

#### Parameters
*Parameters* are inputs which modify how patterns, hits, and signals behave. They are represented as sliders in the UI. 

For example, most patterns have a parameter to change the color or frequency. Alpha is also a parameter.

A key mechanism of Radiance is that parameters can be *attached* MIDI devices or other sources. This allows you to control parameters using MIDI devices. 

##### Connecting Parameters

Clicking on the label of a slider will cause it to turn yellow, at which point it is ready to be connected. Click the label again to reset it.

While the label is yellow, you can click on a filter or signal to attach the parameter to its output. To connect to a MIDI device, you can twiddle the desired knob (or press the desired button) until it connects.

While a parameter is connected to something, its slider knob will change color to indicate what it is connected to. You will not be able to control it with the mouse while it is connected.

To disconnect a parameter, double click on its label.

#### Preview
The preview displays what the pattern or hit looks like on its own and *fully opaque*, as if alpha were set to 1.

### Signals
*Signals* can be used to control parameters in an automatic/programmatic way. 

#### LFO (Low Frequency Oscillator)
The *LFO* acts as a function generator which is synced to the global timebase. 

#### LPF (Low-Pass Filter)
The *LPF* takes in an input and filters it with a "single pole"/"exponential" filter. (`H(z) = (1 - a) / (1 - a z^-1)`)

The filter is actually nonlinear and uses different constants depending on whether the signal is *rising* and *falling*. This can be used to mitigate the inherent phase delay of a linear causal LPF. 

#### AGC (Automatic Gain Control)
The *AGC* also takes in an input and filters it. It attempts to remove the DC component and scale it such that it fills the range of [*Min*, *Max*].

This can be useful to filter the output of the *Onset Detection Function* to fill the entire [0, 1] range. It can be also used as a simple gain & offset by setting *Decay = 0*.

#### Quantile Filter
The *Quantile Filter* also takes in an input and filters it. It can be used to implement a median, maximum, or minimum filter. For median, *Quantile = 0.5*, for maximum *Quantile = 1.0*, etc. 

The *Size* parameter represents how much history to use when computing the quantile value.

This can be useful to filter the output of the *Onset Detection Function* to estimate the "level" of the music (how intense it is).

### Filters
*Filters* also can be used to control parameters. They are the output of VAMP plugins

#### Onset Detection Function
The *ODF* is a key part of the Radiance detection algorithm and describes how likely there is a beat onset at any given time.

The output from the VAMP plugin is run through a single pole diode LPF (like one in the LPF signal). As a result, beats show up as a sharp increase, with an exponential tail.

This function works incredibly well to modulate patterns & hits with, at least with EDM...

#### Fundamental Frequency (F0)
The pYIL VAMP plugin attempts to estimate the fundamental freqnecy (F0) from an audio stream. It's a little weird to do causally, but it is usually useful when the ODF isn't.

The output from the VAMP plugin is run though an AGC stage, like in the AGC signal.

### Master Preview Pane

The square in the upper left is the master preview pane. It is the result of the composition of all the patterns and hits. 

The master preview pane shows what is sampled to be output to the lights.

The colored lines over the pane describe the positions of the strips. Color is used to indicate which strip is which.


Configuration
-------------

Most of the configuration is done by editing the source. Some non-UI configuration is also present in `core/config.def` / `config.ini`. Currently, the defaults in `core/config.def` are used and a copy is dumped to `config.ini`. Slowly constants are being pulled out of `#define`'s and moved into `config.ini`.

### UI
The UI is largely parametrically laid out, and is currently the largest component that can be modified without recompiling. The configuration file `layout.ini` can theoretically be used to "skin" the interface. 

However, currently Radiance just uses the default values defined in `ui/layout.def` and dumps these values to `layout.ini` when run.

#### Headless mode

The UI can be disabled by setting ``[ui] enabled=0`` in the configuration file (`config.ini`). Radiance will then load the first state available and run as a daemon.

### Output Devices (LED Strips)
Currently, Radiance only supports outputting to LED strips over flux. 

LED strips are laid out as a sequence of verticies placed on a [-1, 1]^2 square. The strip id's and these verticies can be configured in `output/slice.h`.

#### Flux

*Note: this feature is deprecated. Use lux instead*

[Flux](https://github.com/zbanks/flux) output can be configured in `config.ini`. Radiance is a flux client, and currently only attempts to control RGB strips.

#### Lux

[lux](https://github.com/ervanalb/lux) output can be configured in `config.ini`. It can be configured to output over UDP or serial. 

### MIDI Devices
Radiance was designed with the Korg NanoKontrol 2 and NanoPad 2 in mind, but would work well with other (slider- and knob-heavy) controllers.

The mapping of MIDI controls to the UI are done in the ``midi.ini`` file specified by ``config.ini``. MIDI Control Change events can be connected to sliders, and Note On/Off/Aftertouch events can be connected to generate events on patterns (see: swipe pattern).

*(Currently, the MIDI mapping system is still being tweaked -- more docs to come soon after it's more concrete)*

### Signals
The signals visible in the UI are defined in the `signals` array in `signals/signal.c`.

### Filters
The filters visible in the UI are defined in the `filters` array in `filters/filter.c`. These correspond to VAMP plugins, which should be in your VAMP path (ex. ``~/vamp/``).

### Default Patterns
The default patterns and hits in the slots are loaded in `core/main.c`.

### Saved State
State is stored in `.ini` files. Currently, only pattern & signal states are saved.

Building
--------

Radiance is built on SDL (1.2, not 2.0), Portaudio, PortMIDI, VAMP, and [flux](http://github.com/zbanks/flux).

Theoretically, you should need the following packages installed on your system (for Ubuntu 15.04):
```
libsdl1.2-dev
libsdl-gfx1.2-dev
libsdl-image1.2-dev
libsdl-ttf2.0-dev
libportaudio-dev
libportmidi-dev
libsamplerate0-dev
libfftw3-dev
vamp-plugin-sdk
libnanomsg-dev
```
...and you'll need to get [flux](http://github.com/zbanks/flux). Compiling this is described on github page.

#### Portmidi
In some versions of Ubuntu, `libportaudio` is out of date and you might need to build a newer version from source if you're having issues compiling.

#### Portmidi
At least in Ubuntu, `libportmidi` is terrible. To make things worse, in Ubuntu/Debian it was [compiled with a debug option](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=765375) which prevents Radiance from catching errors. 

As a result, if your version of portmidi was compiled with this option, Radiance will crash if it encounters any MIDI errors, even if they would be recoverable. It's advised to recompile portmidi from source to prevent this.

#### VAMP Plugins
Finally, although not required to build, you'll want the following VAMP plugins. You can install them to `~/vamp`.

- [BTrack](https://github.com/adamstark/BTrack) - Causal beat detection
- [pYIN](https://code.soundsoftware.ac.uk/projects/pyin) - Pitch estimation
- [Queen Mary Set](http://isophonics.net/QMVampPlugins)


