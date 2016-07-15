#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include "ui/ui.h"
#include "ui/render.h"
#include "util/config.h"
#include "util/err.h"
#include "pattern/deck.h"
#include "pattern/crossfader.h"
#include "midi/midi.h"
#include "audio/audio.h"
#include "audio/analyze.h"
#include "time/timebase.h"
#include "output/output.h"
#include "main.h"

enum loglevel loglevel = LOGLEVEL_INFO;
struct deck deck[N_DECKS];
struct crossfader crossfader;
struct render render;
double time;
double audio_hi;
double audio_mid;
double audio_low;

int main(int argc, char* args[]) {
    config_init(&config);
    config_load(&config, "resources/config.ini");
    loglevel = config.debug.loglevel;

    ui_init();

    for(int i=0; i < N_DECKS; i++) {
        deck_init(&deck[i]);
    }
    crossfader_init(&crossfader);
    render_init(&render, crossfader.tex_output);
    time_init();
    analyze_init();
    audio_start();
    midi_start();
    output_init(&render);

    ui_run();
    ui_term();

    output_term();
    midi_stop();
    audio_stop();
    analyze_term();

    for(int i=0; i < N_DECKS; i++) {
        deck_term(&deck[i]);
    }

    render_term(&render);
    crossfader_term(&crossfader);

    return 0;
}
