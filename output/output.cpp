#include "util/common.h"
#include <unistd.h>

#include "util/config.h"
#include "util/err.h"
#include "util/math.h"
#include "output/output.h"
#include "output/config.h"
#include "output/slice.h"
#include "output/lux.h"

static std::atomic<int> output_running{0};
static std::atomic<int> output_refresh_request{false};
static SDL_Thread * output_thread;
static struct render * render = NULL;
static bool output_on_lux = false;

static int output_reload_devices() {
    // Tear down
    if (output_on_lux) {
        output_lux_term();
    }

    // Reload configuration
    output_on_lux = false;
    int rc = output_config_load(&output_config, config.paths.output_config);
    if (rc < 0) {
        ERROR("Unable to load output configuration");
        return -1;
    }

    // Initialize
    if (output_config.lux.enabled) {
        int rc = output_lux_init();
        if (rc < 0) PERROR("Unable to initialize lux");
        else output_on_lux = true;
    }

    return 0;
}

int output_run(void * args) {
    output_reload_devices();

    /*
    FPSmanager fps_manager;
    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 100);
    */
    double stat_ops = 100;
    int render_count = 0;

    output_running = true;   
    int last_tick = SDL_GetTicks();
    unsigned int last_output_render_count = output_render_count;
    (void) last_output_render_count; //TODO

    while(output_running) {
        if (output_refresh_request) {
            output_reload_devices();
            output_refresh_request = 0;
        }

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
        SDL_Delay(1);
        int tick = SDL_GetTicks();
        int delta = MAX(tick - last_tick, 1);
        stat_ops = INTERP(0.99, stat_ops, 1000. / delta);

        if ((delta < 10) && (delta > 0)) {
            SDL_Delay(10 - delta);
            //LOGLIMIT(DEBUG, "Sleeping for %d ms", 10 - delta);
        }
        last_tick = tick;

        render_count++;
        if (render_count % 101 == 0)
            DEBUG("Output FPS: %0.2f; delta=%d", stat_ops, delta);
    }

    // Destroy output
    if(output_on_lux) output_lux_term();
    output_config_del(&output_config);

    INFO("Output stopped");
    return 0;
}

void output_init(struct render * _render) {
    output_config_init(&output_config);
    render = _render;

    output_thread = SDL_CreateThread(&output_run, "Output", 0);
    if(!output_thread) FAIL("Could not create output thread: %s\n", SDL_GetError());
}

void output_term() {
    output_running = false;
}

void output_refresh() {
    output_refresh_request = true;
}
