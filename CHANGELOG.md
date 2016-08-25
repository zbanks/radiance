20160824 zbanks
==============

- Split config.ini into config.ini & params.ini
    - params.ini can be reloaded
- Reload just params.ini with `r`; reload midi & output with `R`
- Whole decks (as in `decks.ini`) improvements
    - No longer needs the `:` prefix; it just tries to load a deck first.
        - Don't name decks after patterns :(
    - You can set an intensity value in the `decks.ini` file
    - You can leave holes in the deck with `_`
- Stubbing out "soft snap" for MIDI controllers
    - How to resolve position of virtual slider vs. physical slider
    - Not done... yet
