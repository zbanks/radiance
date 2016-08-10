#include "util/common.h"

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

auto current_loglevel = LOGLEVEL_ALL;

loglevel get_loglevel(void)
{
    return current_loglevel;
}
loglevel set_loglevel(loglevel level)
{
    return std::exchange(current_loglevel,level);
}
struct deck deck[N_DECKS];
struct crossfader crossfader;
struct render render;
double current_time;
double audio_hi;
double audio_mid;
double audio_low;
double audio_level;

int main(int argc, char* args[]) {
    config_init(&config);
    config_load(&config, "resources/config.ini");
    current_loglevel = static_cast<loglevel>(config.debug.loglevel);

    ui_init();
    for(auto & d : deck)
        d.init();

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

    for(auto & d : deck)
        d.term();

    render_term(&render);
    crossfader_term(&crossfader);

    return 0;
}
