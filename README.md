Radiance
========

LJ software for controlling light displays. Supports beat detection, animated GIFs, and OpenGL shader effects.

[![Build Status](https://travis-ci.org/zbanks/radiance.svg?branch=master)](https://travis-ci.org/zbanks/radiance)

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

Keyboard Shortcuts
------------------

### Sliders
- `j` - Decrement selected slider by 10%
- `k`  - Increment selected slider by 10%
- `` ` ``, `0-9` - Set selected slider. `` ` `` = 0%; `1` = 10%; `5` = 50%; `0` = 100%

### Nodes
- `r` - Reload node
- Enter - Set node as output
- Delete - Delete the currently selected node

### Other
- `q` - Close output window
- `:` - Load node

Patterns
--------

### Full List
(Generated with ``$ head -n1 resources/effects/*.glsl | xargs -d\n -n3 echo | grep '== //' | sed -e 's|==> resources/effects/\(.*\).glsl <== // \(.*\)$|- `\1` - \2|'`` )

- `allwhite` - Basic white fill 
- `black` - Reduce alpha 
- `bounce` - Zoom in (bounce) to the beat & audio 
- `bstrobe` - Full black strobe. Intensity increases frequency 
- `bwave` - Black sine wave from left to right. 
- `chansep` - Red / green / blue color channel separation 
- `circle` - Yellow blob that spins to the beat 
- `coin` - Rotate the 'object' in 3D, like a coin 
- `crt` - CRT-style effect 
- `cyan` - Cyan diagonal stripes 
- `depolar` - Convert rings to vertical lines 
- `desat` - Desaturate (make white) 
- `desatb` - Desaturate to the beat 
- `diodelpf` - Apply smoothing over time with new hits happening instantly 
- `distort` - Distort the screen to the beat 
- `dunkirk` - Apply a Dunkirk-esque palette 
- `dwwave` - Diagonal white wave 
- `edge` - From https://www.shadertoy.com/view/XssGD7 
- `fire` - Fire from the bottom 
- `fireball` - Fileball in the center 
- `flow` - Radiate color from the center based on audio 
- `flower` - Convert vertical lines to radial flower pattern 
- `fly` - Created by inigo quilez - iq/2013 
- `foh` - First order (expontential) hold 
- `green` - Zero out the everything but the green channel (green is not a creative color) 
- `greenaway` - Shift colors away from green (green is not a creative color) 
- `gstrobe` - Strobe alpha to the beat 
- `heart` - Pink heart 
- `hue` - Shift the color in HSV space 
- `inception` - Created by inigo quilez - iq/2013 
- `interstellar` - From https://www.shadertoy.com/view/Xdl3D2 
- `lpf` - Smooth output 
- `nogreen` - Zero out the green channel (green is not a creative color) 
- `onblack` - Composite the input image onto black 
- `pink` - Pink polka dots 
- `pixelate` - Pixelate/quantize the output 
- `polar` - Convert vertical lines to rings 
- `polygon` - Convert vertical lines to polygon rings 
- `posterize` - Reduce number of colors 
- `purple` - Organic purple waves 
- `qcircle` - Big purple soft circle  
- `rainbow` - Cycle the color (in HSV) over time 
- `random` - Snowcrash: white static noise 
- `randy` - Obnoxiously zoom and rotate 
- `rblur` - Created by inigo quilez - iq/2013 
- `rblurb` - Created by inigo quilez - iq/2013 
- `red` - Change the color (in HSV) to red 
- `resat` - Recolor output with noise rainbow 
- `rjump` - Shift the hue on the beat 
- `rolling` - Rolling shutter effect 
- `rotate` - Rotate the screen 
- `slide` - Slide the screen left-to-right 
- `smoke` - Perlin noise green smoke 
- `snowcrash` - Snowcrash: white static noise 
- `speckle` - Per-pixel twinkle effect 
- `spin` - Rotate the screen 
- `spinb` - Spins the pattern round to the beat 
- `sscan` - White slit for testing 
- `starfield` - Pixels radiating from the center 
- `stripey` - Vertical stripes with a twinkle effect 
- `strobe` - Strobe alpha to the beat 
- `swipe` - Only update a vertical slice that slides across 
- `test` - A green & red circle in the center 
- `threedee` - Fake 3D-glasses effect 
- `tile` - Repeating tiles 
- `tunnel` - From https://www.shadertoy.com/view/4sXSzs 
- `uvmap` - Use .rg as .uv 
- `uvmapcross` - Use .rg as .uv and crossfade 
- `uvmapid` - Base identity pattern for use with `uvmap` 
- `vignette` - Applies vignette 
- `vu` - Blue vertical VU meter 
- `wave` - Green and blue base pattern 
- `wavy` - Distort the screen 
- `wstrobe` - White strobe to the beat 
- `wwave` - White wave with hard edges 
- `yellow` - Yellow and green vertical waves 
- `zoh` - Zero order hold to the beat
- `nyancat.gif`

#### Base patterns

These patterns produce something visually interesting without anything below them.

- `allwhite` - Basic white fill 
- `bwave` - Black sine wave from left to right. 
- `circle` - Yellow blob that spins to the beat 
- `cyan` - Cyan diagonal stripes 
- `dwwave` - Diagonal white wave 
- `fire` - Fire from the bottom 
- `fireball` - Fileball in the center 
- `heart` - Pink heart 
- `interstellar` - From https://www.shadertoy.com/view/Xdl3D2 
- `pink` - Pink polka dots 
- `purple` - Organic purple waves 
- `qcircle` - Big purple soft circle  
- `random` - Snowcrash: white static noise 
- `smoke` - Perlin noise green smoke 
- `snowcrash` - Snowcrash: white static noise 
- `sscan` - White slit for testing 
- `starfield` - Pixels radiating from the center 
- `test` - A green & red circle in the center 
- `uvmapid` - Base identity pattern for use with `uvmap` 
- `vu` - Blue vertical VU meter 
- `wave` - Green and blue base pattern 
- `wstrobe` - White strobe to the beat 
- `wwave` - White wave with hard edges 
- `yellow` - Yellow and green vertical waves 
- `nyancat.gif`

#### Tail patterns

These patterns reduce visual complexity, or at least provide smoothing.

- `desat` - Desaturate (make white) 
- `desatb` - Desaturate to the beat 
- `diodelpf` - Apply smoothing over time with new hits happening instantly 
- `edge` - Spatial edge detect filter (HPF) 
- `edgy` - Fake edge detection based only on alpha 
- `flow` - Radiate color from the center based on audio 
- `foh` - First order (expontential) hold 
- `lpf` - Smooth output 
- `pixelate` - Pixelate/quantize the output 
- `posterize` - Reduce number of colors  (Note: looks bad on strips!)
- `red` - Change the color (in HSV) to red 
- `speckle` - Per-pixel twinkle effect 
- `starfield` - Pixels radiating from the center 
- `stripey` - Vertical stripes with a twinkle effect 
- `zoh` - Zero order hold to the beat


Released under the MIT/X11 License. Copyright 2016 Zach Banks and Eric Van Albert.
