# Contributing

Radiance welcomes all contributions. Good places to get started:

* Writing GLSL effects
* Tackling `easy` issues
* Improving the UI

Additionally, please file an issue if you have any issues building or running radiance. 

## Writing GLSL effects

Currently, radiance has about 80 GLSL effects which live in 
[`resources/effects/*.glsl`](https://github.com/zbanks/radiance/tree/master/resources/library/effects)
files.

A good example is [`vu.glsl`](https://github.com/zbanks/radiance/blob/master/resources/library/effects/vu.glsl).

Also see [`effect_header.glsl`](https://github.com/zbanks/radiance/blob/master/resources/glsl/effect_header.glsl)
to see what uniforms and functions are available to use.

## Tackling `easy` issues

Issues that are small, well-scoped tasks are marked as `easy`.
[Check out the queue](https://github.com/zbanks/radiance/issues?q=is%3Aopen+is%3Aissue+label%3Aeasy)
to see if there's anything you want to tackle.

## Improving the UI

The UI is mostly written in 
[QML](https://github.com/zbanks/radiance/tree/master/resources/qml),
with some more complex functionality implemented in
[View.cpp](https://github.com/zbanks/radiance/tree/master/src/View.cpp).

This is a good place to make incremental improvements, and both @zbanks and @ervanalb suck at this stuff.
