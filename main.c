#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include "ui/ui.h"
#include "util/config.h"
#include "pattern/deck.h"
#include "main.h"

struct deck deck[N_DECKS];

int main(int argc, char* args[]) {
    config_init(&config);
    config_load(&config, "resources/config.ini");

    ui_init();

    for(int i=0; i < N_DECKS; i++) {
        deck_init(&deck[i]);
    }
    deck_load_pattern(&deck[0], 0, "resources/patterns/test");
    deck_load_pattern(&deck[0], 1, "resources/patterns/test");

    ui_run();
    ui_close();

    for(int i=0; i < N_DECKS; i++) {
        deck_term(&deck[i]);
    }

    return 0;
}
