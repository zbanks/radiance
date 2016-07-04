Radiance Todo List
==================

Core
----

- MIDI Controllers
- Output/rendering integration into main loop

UI
--

- Remove `#define GL_GLEXT_PROTOTYPES`
- Autocomplete/matching of pattern names
- More decks
- Strip output visualization / editing


Patterns
--------

- Document/re-write existing patterns
- Color palette patterns?
- Speckle

Audio / Beats
-------------

- Bar tracking
- Remove double FFT (better integration with BTrack)
- Expose controls to fix tempo
- Disable audio beat tracking and use wall time + fixed tempo

Output / Lux
------------

- Proper channel-agnostic enumeration
- Pixel list -> frame payload
- `FRAME_HOLD` + `SYNC`
- Stat collection (`PKTCNT` commands)
- Reloading device list/configuration

Misc
----

- Headless mode

