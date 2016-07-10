#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <unistd.h>

#include "util/config.h"
#include "util/err.h"
#include "util/math.h"
#include "output/output.h"
#include "output/config.h"
#include "output/slice.h"
#include "output/lux.h"

static volatile int output_running;
static SDL_Thread * output_thread;
static struct render * render = NULL;

int output_run(void * args) {
    bool output_on_lux = false;
    if (config.output.lux_enabled) {
        int rc = output_lux_init();
        if (rc < 0) PERROR("Unable to initialize lux");
        else output_on_lux = true;
    }

    /*
    FPSmanager fps_manager;
    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 100);
    */
    double stat_ops = 100;

    output_running = true;   
    unsigned int last_tick = SDL_GetTicks();
    unsigned int last_output_render_count = output_render_count;
    (void) last_output_render_count; //TODO

    while(output_running) {
        //if (last_output_render_count == output_render_count)
        last_output_render_count = output_render_count;
        int rc = output_render(render);
        if (rc < 0) PERROR("Unable to render");

        if (output_on_lux) {
            int rc = output_lux_prepare_frame();
            if (rc < 0) PERROR("Unable to prepare lux frame");
            rc = output_lux_sync_frame();
            if (rc < 0) PERROR("Unable to sync lux frame");
        }

        //SDL_framerateDelay(&fps_manager);
        stat_ops = INTERP(0.8, stat_ops, 1000. / (SDL_GetTicks() - last_tick));
        last_tick = SDL_GetTicks();
        SDL_Delay(10);
    }

    // Destroy output
    if(output_on_lux) output_lux_term();
    output_config_del(&output_config);

    INFO("Output stopped");
    return 0;
}

void output_init(struct render * _render) {
    output_config_init(&output_config);
    int rc = output_config_load(&output_config, config.output.config);
    if (rc < 0) FAIL("Unable to load configuration");
    //output_config_dump(&output_config, config.output.config);
    render = _render;

    output_thread = SDL_CreateThread(&output_run, "Output", 0);
    if(!output_thread) FAIL("Could not create output thread: %s\n", SDL_GetError());
}

void output_term() {
    output_running = false;
}
