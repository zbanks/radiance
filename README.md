Radiance
========

Radiance is video art software designed for live performance. More information is available at [radiance.video](https://radiance.video/)

[![Build Status](https://travis-ci.org/zbanks/radiance.svg?branch=master)](https://travis-ci.org/zbanks/radiance)

Features
--------

### GPU-based shader effects

Chain together simple building blocks to create fantastic video effects. All rendering is GPU-accelerated for good performance, even on laptops. Radiance comes pre-loaded with over 150 effects, which are written in WGSL and can be edited live.

### Media

Bring your own video clips and images. Radiance integrates the powerful video player MPV. Open files, webcams, live streams, and even Youtube videos, straight into Radiance.

### Beat detection

Connect Radiance to your sound system for music-reactive visuals. Radiance performs beat detection on the incoming audio, and uses this as the global timebase. This means that every effect will automatically be "on the beat" and will slow down or speed up along with the music.

### Performance-ready
Radiance is designed with live performance in mind. Patterns are easy to create and tweak on the fly. Output to one or more screens or use the built-in projection mapping editor.

Project Status
--------------

Radiance has recently been completely rewritten in rust. The rewrite is not quite at feature-parity, and release builds is are not yet available.
However, you are encouraged to try building radiance from source, and should fine that experience much improved.

### Missing features from 0.6.1:
* MIDI controller input
* TCP output (for LED displays)

Download
--------

You can download old releases (circa 2019) of Radiance for Linux or MacOS from the [releases](https://github.com/zbanks/radiance/releases) page.

If you are having trouble with those, you are encouraged to try building from source, until a modern release is available--see the next section. I've only tried Linux but I see no reason why it wouldn't work on Windows or MacOS.

Build
-----

### Dependencies

- `rust` and `cargo`

### Building Radiance

    git clone https://github.com/zbanks/radiance.git
    cd radiance
    cargo run

### youtube-dl

Radiance uses `libmpv` to load videos, which can optionally use `yt-dlp` to stream videos from YouTube and many other sites. Since `yt-dlp` updates frequently, we have avoided bundling it with Radiance. Instead, on Linux:

    sudo pip install yt-dlp

or on Mac:

    brew install yt-dlp

You can then load YouTube videos into Radiance by URL.

Mouse Control
-------------

- `Click` (on a tile) - select tile
- `Ctrl`-`click` (on a tile) - select multiple
- `Shift`-`click` (on a tile) - select contiguous
- `Click and drag` (on a tile) - re-order selected tiles
- `Scroll wheel` - change intensity of selected tiles

Keyboard Control
----------------

- `A` - Add a node from the library 
- `Delete` - Remove a tile
- `R` - Reload tile

Shader Effects
--------------

Radiance generates video from a set of connected nodes. Nodes are based on WGSL fragment shaders, but can also be static images or videos. Each node takes one or more inputs and produces exactly one output.

Each WGSL fragment shader node is described by a single `.wgsl` file in [`library/`](https://github.com/zbanks/radiance/tree/master/library/).

For the most part, these files are plain WGSL describing the fragment shader. This is similar to setups on shadertoy.com or glslsandbox.com . Each shader defines a function `fn main(uv: vec2<f32>) -> vec4<f32>` which returns a pixel color for a given coordinate. The return value should be a RGBA color with pre-multiplied alpha: so white with 40% opacity is encoded as `vec4<f32>(0.4, 0.4, 0.4, 0.4)`. The coodinate `uv` has `x` and `y` values in the range `[0.0, 1.0]` with `vec2<f32>(0., 0.)` corresponding to the lower-left corner.

The shader also has access to its input(s) as textures through the `iInputTex` (or `iInputsTex[]`) bindings.

In addition, each shader has access to uniforms which are documented in [`library/*.wgsl`](https://github.com/zbanks/radiance/tree/master/library/). The most important is `iIntensity`, which is a value in the range `[0.0, 1.0]` and is set by the effect's slider in the UI. There is a limitation of having *exactly one* input slider per effect: this is intentional to reduce the cognitive overhead on the end user. Other variables include information about the current audio and time.

### Invariants

Each shader must follow these properties:

* The returned color set by each shader must be a valid, pre-multiplied alpha, RGBA tuple. Each component of the `vec4` must be in the range `[0.0, 1.0]`, and the RGB components must be less than or equal to the A component. (See `afixhighlight` for a shader that will highlight errors here in pink)
* Identity: the shader must pass through its first input completely unchanged when `iIntensity == 0.` This means that inserting a new shader should not affect the output until its intensity is increased.

### Multi-buffer Shader Effects

Some effects cannot be accomplished with a single fragment shader pass. An effect can consist of a series of shaders, separated by `#buffershader`. Each shader renders to a texture in `iChannelsTex[]` (e.g. the first shader renders to `iChannelsTex[0]`, which, if accessed in the shader, contains the previous output frame.) The shaders are rendered in backwards-order, so the last shader in the file is rendered first. Only the output of the first shader in the file is displayed -- the other buffers persist between frames but are not exposed to other nodes.

An example that uses this feature is [`scramble.wgsl`](https://github.com/zbanks/radiance/blob/master/library/scramble.wgsl). This shader shifts around parts of the input. In order to persist the UV map of how much each pixel is shifted, it uses one buffer shader whose output persists in `iChannelsTex[1]`.

Copyright & License
-------------------

Released under the MIT/X11 License. Copyright 2016 Zach Banks and Eric Van Albert.

