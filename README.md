Radiance
========

Radiance is video art software for VJs. It supports beat detection, animated GIFs, YouTube video, and OpenGL shader effects.

[![Build Status](https://travis-ci.org/zbanks/radiance.svg?branch=master)](https://travis-ci.org/zbanks/radiance)

You can download this software for MacOS from (https://radiance.video radiance.video). There are no prebuilt Linux packages, but you can build it yourself fairly easily.

Screenshots
-----------

![screenshot](https://i.imgur.com/hgdTxPU.png)

![GIF Screencapture](https://i.imgur.com/I4qnMQo.gif)

[Example effects](https://zbanks.github.io/radiance/)

Build
-----

### Dependencies

- `Qt 5.6`
- `SDL2-TTF`
- `PortAudio`
- `FFTW3`
- `libsamplerate`

### Optional Dependencies
- `libmpv`
- `rtmidi`

Install dependencies on Ubuntu:

    $ sudo apt-add-repository ppa:beineri/opt-qt591-trusty
    $ sudo apt-get update
    $ sudo apt-get install qt59base qt59multimedia qt59quickcontrols qt59imageformats qt59quickcontrols2 qt59script libfftw3-dev libsamplerate0-dev libasound2-dev libmpv-dev
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
    ./radiance_cli      # Command line GIF generator

If you `git pull` changes, make sure you also do `git submodule update` to pull in changes to `BTrack/`.

### youtube-dl

Radiance uses `libmpv` to load videos, which can optionally use `youtube-dl` to stream videos from YouTube and many other sites. Since `youtube-dl` updates frequently, we have avoided bundling it with Radiance. Instead, on Linux:

    sudo pip install youtube-dl

or on Mac:

    brew install youtube-dl

You can then load YouTube videos into Radiance by typing `youtube:search terms` into the pattern loader box.

Keyboard Shortcuts
------------------

### Tiles
- `j` - Decrement selected slider by 10%
- `k`  - Increment selected slider by 10%
- `` ` ``, `0-9` - Set selected slider. `` ` `` = 0%; `1` = 10%; `5` = 50%; `0` = 100%
- `Delete` - Remove a tile
- `Enter` - Set a tile as output (then click `Show Output` to fullscreen it)
- `Ctrl` + `` ` ``, `0-9` - Assign slider to MIDI knob
- `r` - Reload node

### Other
- `q` - Close output window
- `:` - Load node

Released under the MIT/X11 License. Copyright 2016 Zach Banks and Eric Van Albert.
