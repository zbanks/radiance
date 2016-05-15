#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include "ui/ui.h"
#include "util/config.h"

int main(int argc, char* args[]) {
    config_init(&config);
    config_load(&config, "config.ini");

    ui_init();
    ui_run();
    ui_close();

    return 0;
}
