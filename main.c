#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include "ui/ui.h"
#include "util/config.h"
#include "deck/deck.h"

struct deck deck[1];

int main(int argc, char* args[]) {
    config_init(&config);
    config_load(&config, "resources/config.ini");

    ui_init();

    deck_init(&deck[0]);
    deck_load_pattern(&deck[0], 0, "resources/patterns/test");

    ui_run();
    ui_close();

    deck_term(&deck[0]);

    return 0;
}
