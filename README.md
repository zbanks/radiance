NOTES FOR THIS FORK
===================
This fork is an attempt to revive this project. `qthelper.hpp` has been added under `src/` from [mpv-examples](https://github.com/mpv-player/mpv-examples/blob/bd0b42e5bd47c22592760a244f80e49ec0222892/libmpv/common/qthelper.hpp) which is covered under GPL-2.0, LGPL-2.1 licenses.

Running with Docker
-------------------

The aim of this dockerfile is to build and run Radiance through docker alone. For that reason, since we will be running an X window through docker, you probably will have to first `docker` to `xhost` with:

```$ xhost +local:docker```

After that, you can simply run:
```
$ git clone https://github.com/iakovts/radiance
$ git checkout dockerfile_supp
$ git submodule update --init
$ docker-compose build
$ docker-compose run
```

and Radiance will run. 

> Note: There still are some issues with stability which need work.

Radiance
========

Radiance is video art software for VJs. It supports beat detection, animated GIFs, YouTube video, OpenGL shader effects. It is designed for live performance and runs on Linux and MacOS.

[![Build Status](https://travis-ci.org/zbanks/radiance.svg?branch=master)](https://travis-ci.org/zbanks/radiance)

Artwork
-------

![Simple screencapture](https://i.imgur.com/I4qnMQo.gif)

[Sample Artwork 1](https://i.imgur.com/R4tsFfG.gifv)

[Sample Artwork 2](https://i.imgur.com/B0QMoXT.gifv)

[Sample Artwork 3](https://i.imgur.com/4FON4vY.gifv)

[Screencapture from this artwork](https://i.imgur.com/Vb1yPZl.gifv)

[Example effects](https://radiance.video/library/)

Download
--------

You can download Radiance for Linux or MacOS from the [releases](https://github.com/zbanks/radiance/releases) page.

Build
-----

### Dependencies

- `Qt 5.9` or higher
- `PortAudio`
- `FFTW3`
- `libsamplerate`

### Optional Dependencies
- `libmpv`
- `rtmidi`

Install dependencies on Ubuntu:

    $ sudo apt-add-repository ppa:beineri/opt-qt591-trusty
    $ sudo apt-get update
    $ sudo apt-get install qt59base qt59multimedia qt59quickcontrols qt59imageformats qt59quickcontrols2 qt59script libfftw3-dev libsamplerate0-dev libasound2-dev libmpv-dev libdrm-dev libgl1-mesa-dev
    $ git clone https://github.com/EddieRingle/portaudio    # build & install
    $ git clone https://github.com/thestk/rtmidi            # build & install

Note: you may need to install portaudio & rtmidi from git as above

### Building Radiance

    git clone https://github.com/zbanks/radiance
    git submodule update --init
    cd radiance
    mkdir build
    cd build
    cmake .. # -DCMAKE_PREFIX_PATH=/opt/qt59/ -DCMAKE_BUILD_TYPE=Debug
    make
    ./radiance          # Qt UI
    ./radiance_cli      # Command line  GIF generator

If you `git pull` changes, make sure you also do `git submodule update` to pull in changes to `BTrack/`.

### youtube-dl

Radiance uses `libmpv` to load videos, which can optionally use `youtube-dl` to stream videos from YouTube and many other sites. Since `youtube-dl` updates frequently, we have avoided bundling it with Radiance. Instead, on Linux:

    sudo pip install youtube-dl

or on Mac:

    brew install youtube-dl

You can then load YouTube videos into Radiance using the youtube node in the library.

Mouse Control
-------------

- `Double click` (in the library) - add tile
- `Click` (on a tile) - select tile
- `Ctrl`-`click` (on a tile) - select multiple
- `Shift`-`click` (on a tile) - select contiguous
- `Click and drag` (on a tile) - re-order selected tiles
- `Scroll wheel` - change intensity of selected tiles
- `Ctrl`-`scroll wheel` - zoom

Keyboard Control
----------------

- `:` - Search library
- `Ctrl`-`+` - Zoom in
- `Ctrl`-`-` - Zoom out
- `Ctrl`-`0` - Reset zoom
- `` ` ``, `0-9` - Set selected slider. `` ` `` = 0%; `1` = 10%; `5` = 50%; `0` = 100%
- `j` - Decrement selected slider by 10%
- `k`  - Increment selected slider by 10%
- `Delete` - Remove a tile
- `Ctrl` + `` ` ``, `1-9` - Assign slider to MIDI knob
- `r` - Reload tile
- `Esc` - Close output window

Shader Effects
--------------

Radiance generates video from a set of connected "VideoNodes." Most commonly, these nodes are based on OpenGL fragment shaders, but can also be static images, GIFs, or videos. Each node takes one or more inputs and produces exactly one output.

Each OpenGL fragment shader node is described by a single `.glsl` file in [`resources/library/effects`](https://github.com/zbanks/radiance/tree/master/resources/library/effects).

For the most part, these files are plain GLSL describing the fragment shader. This is similar to setups on shadertoy.com or glslsandbox.com . Each shader defines a function `void main(void)` which sets a pixel color `vec4 fragColor` for a given coordinate `vec2 uv`. `fragColor` is an RGBA color with pre-multiplied alpha: so white with 40% opacity is encoded as `vec4(0.4, 0.4, 0.4, 0.4)`. The coodinate `uv` has `x` and `y` values in the range `[0.0, 1.0]` with `vec2(0., 0.)` corresponding to the lower-left corner.

The shader also has access to its input(s) as textures through the `iInput` (or `iInputs[]`) uniforms.

In addition, each shader has access to additional uniforms which are documented in [`resources/effects/*.glsl`](https://github.com/zbanks/radiance/tree/master/resources/library/effects). The most important is `iIntensity`, which is a value in the range `[0.0, 1.0]` which is mapped to a slider in the UI that the user controls. There is a limitation of having *exactly one* input slider per effect: this is intentional to reduce the cognitive overhead on the end user. Other variables include information about the current audio or time.

### Invariants

Each shader must follow these properties:

* The `fragColor` set by each shader must be a valid, pre-multiplied alpha, RGBA tuple. Each component of the `vec4` must be in the range `[0.0, 1.0]`, and the RGB components must be less than or equal to the A component. (See `afixhighlight` for a shader that will highlight errors here in pink)
* Identity: the shader must pass through its first input completely unchanged when `iIntensity == 0.` This means that inserting a new shader should not affect the output until its intensity is increased.


### Multi-buffer Shader Effects

Some effects cannot be accomplished with a single fragment shader pass. An effect can consist of a series of shaders, separated by `#buffershader`. Each shader renders to a texture in `iChannels[]` (e.g. the first renders to `iChannels[0]`). The shaders are rendered in backwards-order, so the last shader in the file is rendered first. Only the output of the first shader in the file is displayed -- the other buffers persist between frames but are not exposed to other nodes.

An example that uses this feature is [`foh.glsl`](https://github.com/zbanks/radiance/blob/master/resources/library/effects/foh.glsl). This implements an (exponential) "first-order hold" - it samples the input texture on a multiple of the beat and stores it in `iChannel[1]`.



Copyright & License
-------------------

Released under the MIT/X11 License. Copyright 2016 Zach Banks and Eric Van Albert.

