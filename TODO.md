Radiance TODO list
==================

Shader porting
--------------

* 73 / 174

Big features
------------

* [X] Screen output nodes (they are sort of hacked in right now)
* [X] Auto-DJ
* [X] Multi-screen output node (for multiple projectors)
* [X] Image node

Small features / bugs
---------------------

* [ ] Inserting a node should focus & select it (see TODO in main.rs)
* [ ] Reload node with "R" should preserve textures
* [ ] Shader compilation should be done in background
* [ ] Implement "shift"-selection (select range)
* [X] Compute audio levels and pass to effects
* [ ] Bouncing ball UI widget
* [X] Spectrogram UI widget
* [X] Waveform UI widget
* [ ] Make mosaic scrollable

Nice to have
------------

* [ ] Invoke shader compiler only once for multi-channel effects
* [ ] UI affordance for forking an output path (allow user to DAG-ify the graph)
* [ ] Better drag & drop behavior for DAGs (see TODO in mosaic.rs)
* [ ] Speed up compilation by moving beat tracking NN weights to files that are loaded at runtime
* [ ] Package for crates.io (need to figure out how to integrate custom EGUI)
* [ ] Failed shader compilation should show logs on-screen

Low-priority big features
-------------------------

* [ ] Node Library
* [ ] Animated GIF support for image node
* [X] Movie node (with libmpv)
* [ ] MIDI controller support
