#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include "ui/ui.h"
#include "util/config.h"
#include "pattern/deck.h"
#include "pattern/crossfader.h"
#include "audio/audio.h"
#include "audio/analyze.h"
#include "main.h"

struct deck deck[N_DECKS];
struct crossfader crossfader;
double time;

int main(int argc, char* args[]) {
    config_init(&config);
    config_load(&config, "resources/config.ini");

    ui_init();

    for(int i=0; i < N_DECKS; i++) {
        deck_init(&deck[i]);
    }
    crossfader_init(&crossfader);

    analyze_init();
    audio_start();

    ui_run();
    ui_term();

    audio_stop();
    analyze_term();

    for(int i=0; i < N_DECKS; i++) {
        deck_term(&deck[i]);
    }

    return 0;
}
