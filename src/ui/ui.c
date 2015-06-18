#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_framerate.h>

#include "ui/ui.h"
#include "ui/layout.h"
#include "ui/graph.h"
#include "ui/waveform.h"
#include "ui/slider.h"
#include "ui/text.h"
#include "core/config.h"
#include "core/err.h"
#include "core/parameter.h"
#include "core/slot.h"
#include "state/state.h"
#include "dynamic/object.h"
#include "filters/filter.h"
#include "waveform/waveform.h"
#include "midi/midi.h"
#include "output/slice.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "signals/signal.h"
#include "timebase/timebase.h"

#define SURFACES \
    X(master_preview, layout.master) \
    X(pattern_preview, layout.slot.preview_rect) \
    X(audio_pane, layout.audio) \
    X(slot_deck_pane, layout.slot_deck) \
    X(slot_pane, layout.slot) \
    X(pattern_pane, layout.add_pattern) \
    X(signal_pane, layout.signal) \
    X(filter_pane, layout.filter) \
    X(output_pane, layout.output) \
    X(midi_pane, layout.midi) \
    X(midi_reload_pane, layout.midi_reload) \
    X(palette_pane, layout.palette) \
    X(palette_preview, layout.palette.preview_rect) \
    X(state_panel_pane, layout.state_panel) \
    X(state_save_pane, layout.state_save) \
    X(state_load_pane, layout.state_load)

#define X(s, l) \
    static SDL_Surface* s;
SURFACES
#undef X

static SDL_Surface * screen;

static int mouse_is_down;
static struct xy mouse_drag_start_abs;

struct xy mouse_drag_delta;
struct xy mouse_drag_start;

void (*mouse_drag_fn_p)();
void (*mouse_drop_fn_p)();

int active_pattern;
int active_slot;
slot_t* active_preview;

param_state_t * active_param_source;
struct colormap ** active_palette_source;

static size_t master_pixels;
static float * master_xs;
static float * master_ys;
static color_t * master_frame;

static void (*ui_done_fn)();
static int ui_running;
static SDL_Thread* ui_thread;

SDL_Surface * ui_create_surface_or_die(int width, int height){
    SDL_Surface * s = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0); \
    if(!s) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
    return s;
}

static void ui_init()
{
    if (TTF_Init())
    {
        FAIL("TTF_Init Error: %s\n", SDL_GetError());
    }

    screen = SDL_SetVideoMode(layout.window.w, layout.window.h, 0, SDL_DOUBLEBUF);
    // | SDL_ANYFORMAT | SDL_FULLSCREEN | SDL_HWSURFACE);

    if (!screen) FAIL("SDL_SetVideoMode Error: %s\n", SDL_GetError());

#define X(s, l) \
    s = ui_create_surface_or_die(l.w, l.h);
SURFACES
#undef X

    slider_init();
    graph_init();
    ui_waveform_init();

    mouse_is_down = 0;
    mouse_drag_fn_p = 0;
    mouse_drop_fn_p = 0;
    active_pattern = -1;
    active_slot = -1;
    active_preview = 0;

    master_pixels = layout.master.w * layout.master.h;
    master_xs = malloc(sizeof(float) * master_pixels);
    master_ys = malloc(sizeof(float) * master_pixels);
    master_frame = malloc(sizeof(color_t) * master_pixels);

    if(!master_xs) FAIL("Unable to alloc xs for master.\n");
    if(!master_ys) FAIL("Unable to alloc ys for master.\n");
    if(!master_frame) FAIL("Unable to alloc frame for master.\n");

    int i = 0;
    for(int y=0;y<layout.master.h;y++) {
        for(int x=0;x<layout.master.w;x++) {
            master_xs[i] = ((float)x / (layout.master.w - 1)) * 2 - 1;
            master_ys[i] = ((float)y / (layout.master.h - 1)) * 2 - 1;
            i++;
        }
    }
}

void ui_quit()
{
    for(int i=0; i<n_slots; i++)
    {
        pat_unload(&slots[i]);
    }

#define X(s, l) \
    SDL_FreeSurface(s);
SURFACES
#undef X

    free(master_xs);
    free(master_ys);
    free(master_frame);

    text_unload_fonts();

    slider_del();
    graph_del();
    ui_waveform_del();

    SDL_Quit();
}

static int x_to_px(float x)
{
    return (int)(((x + 1) / 2) * layout.master.w);
}

static int y_to_px(float y)
{
    return (int)(((y + 1) / 2) * layout.master.h);
}

static int SDL_line(SDL_Surface* dst, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if(x1 == x2) return vlineRGBA(dst, x1, y1, y2, r, g, b, a);
    if(y1 == y2) return hlineRGBA(dst, x1, x2, y1, r, g, b, a);
    return lineRGBA(dst, x1, y1, x2, y2, r, g, b, a);
}

static void ui_draw_button(SDL_Surface * surface, SDL_Color bg_color, struct txt * label_fmt, const char * label){
    rect_t r = {.x = 0, .y = 0, .w = surface->w, .h = surface->h};
    SDL_FillRect(surface, &r, map_sdl_color(surface, bg_color));
    text_render(surface, label_fmt, 0, label);
}

static void update_master_preview() {
    SDL_LockSurface(master_preview);

    render_composite_frame(STATE_SOURCE_UI, master_xs, master_ys, master_pixels, master_frame);
    int i = 0;
    for(int y=0;y<layout.master.h;y++) {
        for(int x=0;x<layout.master.w;x++) {
            ((uint32_t*)(master_preview->pixels))[i] = SDL_MapRGB(
                master_preview->format,
                (uint8_t)roundf(255 * master_frame[i].r),
                (uint8_t)roundf(255 * master_frame[i].g),
                (uint8_t)roundf(255 * master_frame[i].b));
            i++;
        }
    }

    SDL_UnlockSurface(master_preview);

    for(i=0; i<n_output_strips; i++)
    {
        int x1;
        int y1;
        output_vertex_t* v = output_strips[i].first;
        SDL_Color c = output_strips[i].color;

        int x2 = x_to_px(v->x);
        int y2 = y_to_px(v->y);
        for(v = v->next; v; v = v->next)
        {
            x1 = x2;
            y1 = y2;
            x2 = x_to_px(v->x);
            y2 = y_to_px(v->y);

            SDL_line(master_preview,
                     x1, y1,
                     x2, y2,
                     c.r, c.g, c.b, 255);
        }
    }
}

static void update_pattern_preview(slot_t* slot) {
    SDL_LockSurface(pattern_preview);

    for(int x = 0; x < layout.slot.preview_w; x++)
    {
        for(int y = 0; y < layout.slot.preview_h; y++)
        {
            // Checkerboard background
            int i = (x / 10) + (y / 10);
            float bg_shade = (i & 1) ? 0.05 : 0.35;

            float xf = ((float)x / (layout.slot.preview_w - 1)) * 2 - 1;
            float yf = ((float)y / (layout.slot.preview_h - 1)) * 2 - 1;
            color_t pixel = (*slot->pattern->render)(slot->ui_state, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.slot.preview_w * y] = SDL_MapRGB(
                pattern_preview->format,
                (uint8_t)roundf(255 * (pixel.r * pixel.a + (1.0 - pixel.a) * bg_shade)),
                (uint8_t)roundf(255 * (pixel.g * pixel.a + (1.0 - pixel.a) * bg_shade)),
                (uint8_t)roundf(255 * (pixel.b * pixel.a + (1.0 - pixel.a) * bg_shade)));
        }
    }

    SDL_UnlockSurface(pattern_preview);
}

static void ui_update_slot(slot_t* slot) {
    rect_t r;
    rect_array_origin(&layout.slot.rect_array, &r);
    SDL_FillRect(slot_pane, &r, map_sdl_color(slot_pane, layout.slot.bg_color));

    if(slot->pattern)
    {
        update_pattern_preview(slot);
        SDL_BlitSurface(pattern_preview, 0, slot_pane, &layout.slot.preview_rect);

        const char * palette_str;
        if(slot->colormap)
            palette_str = slot->colormap->name;
        else
            palette_str = "Global";

        if(&slot->colormap == active_palette_source)
            text_render(slot_pane, &layout.slot.palette_txt, &layout.slot.palette_highlight_color, palette_str);
        else
            text_render(slot_pane, &layout.slot.palette_txt, 0, palette_str);

        slider_render_alpha(&slot->alpha);
        SDL_BlitSurface(alpha_slider_surface, 0, slot_pane, &layout.slot.alpha_rect);

        for(int i = 0; i < slot->pattern->n_params; i++)
        {
            if(&slot->param_states[i] == active_param_source){
                slider_render(&slot->pattern->parameters[i], &slot->param_states[i], layout.slider.highlight_color);
            }else{
                slider_render(&slot->pattern->parameters[i], &slot->param_states[i], layout.slider.name_color);
            }
            rect_array_layout(&layout.slot.sliders_rect_array, i, &r);

            SDL_BlitSurface(slider_surface, 0, slot_pane, &r);
        }
        
    }
}

static void ui_update_pattern(pattern_t* pattern) {
    rect_t r;
    rect_array_origin(&layout.add_pattern.rect_array, &r);
    SDL_FillRect(pattern_pane, &r, map_sdl_color(pattern_pane, layout.add_pattern.bg_color));

    text_render(pattern_pane, &layout.add_pattern.name_txt, 0, pattern->name);
}

static void ui_update_slot_deck(){
    rect_t r;
    rect_origin(&layout.slot_deck.rect, &r);
    SDL_FillRect(slot_deck_pane, &r, map_sdl_color(slot_deck_pane, layout.slot_deck.bg_color));

    for(int i = 0; i < layout.slot.n_divider_colors; i++){
        SDL_Color c = layout.slot.divider_colors[i];
        rect_array_layout(&layout.slot.rect_array, i+1, &r);
        r.x -= (layout.slot.px - layout.slot.w) / 2 + 1;
        vlineRGBA(slot_deck_pane, r.x - 1, r.y, r.y + r.h, c.r, c.g, c.b, 255);
        vlineRGBA(slot_deck_pane, r.x, r.y - 1, r.y + r.h + 1, c.r, c.g, c.b, 255);
        vlineRGBA(slot_deck_pane, r.x + 1, r.y, r.y + r.h, c.r, c.g, c.b, 255);
    }

    // Render slots
    for(int i=0; i<n_slots; i++)
    {
        if(slots[i].pattern && i != active_slot)
        {
            ui_update_slot(&slots[i]);
            rect_array_layout(&layout.slot.rect_array, i, &r);

            SDL_BlitSurface(slot_pane, 0, slot_deck_pane, &r);
        }
    }

    // Render pattern selection buttons
    for(int i=0; i<n_patterns; i++)
    {
        if(active_pattern != i)
        {
            ui_update_pattern(patterns[i]);
            rect_array_layout(&layout.add_pattern.rect_array, i, &r);
            SDL_BlitSurface(pattern_pane, 0, slot_deck_pane, &r);
        }
    }

    // Render floating slot
    if(active_slot >= 0)
    {
        ui_update_slot(&slots[active_slot]);
        rect_array_layout(&layout.slot.rect_array, active_slot, &r);
        rect_shift(&r, &mouse_drag_delta);
        SDL_BlitSurface(slot_pane, 0, slot_deck_pane, &r);
    }

    // Render floating pattern
    if(active_pattern >= 0)
    {
        ui_update_pattern(patterns[active_pattern]);
        rect_array_layout(&layout.add_pattern.rect_array, active_pattern, &r);
        rect_shift(&r, &mouse_drag_delta);
        SDL_BlitSurface(pattern_pane, 0, slot_deck_pane, &r);
    }
}

static void ui_update_audio_panel(){
    rect_t r;
    rect_origin(&layout.audio.rect, &r);
    SDL_FillRect(audio_pane, &r, map_sdl_color(audio_pane, layout.audio.bg_color));

    ui_waveform_render();
    SDL_BlitSurface(waveform_surface, 0, audio_pane, &layout.waveform.rect);

    mbeat_t time = timebase_get(); 
    float phase = MB2B(time % 4000);
    //float dx = 1.0 - fabs(phase - 1.0);
    float dx = 1.0 - fabs(fmod(phase, 2.0) - 1.0);
    float dy = 4. * (dx - 0.5) * (dx - 0.5);
    int y;
    if(phase > 3.)
        y = layout.audio.ball_y + layout.audio.ball_r + (layout.audio.ball_h - 2 * layout.audio.ball_r) * dy;
    else
        y = layout.audio.ball_y + layout.audio.ball_r + (layout.audio.ball_h - 2 * layout.audio.ball_r) * (0.4 + 0.6 * dy);

    int x = layout.audio.ball_x + layout.audio.ball_r + (layout.audio.ball_w - 2 * layout.audio.ball_r) * 0.5;
    SDL_Color ball_c = layout.audio.ball_color;
    filledCircleRGBA(audio_pane, x, y, layout.audio.ball_r, ball_c.r, ball_c.g, ball_c.b, 255);

    SDL_Color bf_c = layout.audio.ball_floor_color;
    hlineRGBA(audio_pane, layout.audio.ball_x, layout.audio.ball_x + layout.audio.ball_w, layout.audio.ball_y + layout.audio.ball_h, bf_c.r, bf_c.g, bf_c.b, 255);

    char buf[16];
    snprintf(buf, 16, "bpm: %.2f", timebase_get_bpm());
    text_render(audio_pane, &layout.audio.bpm_txt, 0, buf);

    snprintf(buf, 16, "fps: %d", stat_fps);
    text_render(audio_pane, &layout.audio.fps_txt, 0, buf);

    snprintf(buf, 16, "ops: %d", stat_ops);
    text_render(audio_pane, &layout.audio.ops_txt, 0, buf);

    snprintf(buf, 16, "%lu", ((time % 4000) / 1000) + 1);
    text_render(audio_pane, &layout.audio.beat_txt, 0, buf);

    rect_array_layout(&layout.audio.bins_rect_array, WF_HIGH, &r);
    SDL_FillRect(audio_pane, &r, map_sdl_color(audio_pane, layout.waveform.highs_color));
    rect_array_layout(&layout.audio.bins_rect_array, WF_MID, &r);
    SDL_FillRect(audio_pane, &r, map_sdl_color(audio_pane, layout.waveform.mids_color));
    rect_array_layout(&layout.audio.bins_rect_array, WF_LOW, &r);
    SDL_FillRect(audio_pane, &r, map_sdl_color(audio_pane, layout.waveform.lows_color));

    SDL_Color tbs_c = layout.audio.btrack_off_color;
    if(timebase_source == TB_AUTOMATIC) {
        tbs_c = layout.audio.btrack_on_color;
    }
    boxRGBA(audio_pane, layout.audio.auto_x, layout.audio.auto_y, layout.audio.auto_x + layout.audio.auto_w, layout.audio.auto_y + layout.audio.auto_h, tbs_c.r, tbs_c.g, tbs_c.b, 255);
}

static void ui_update_filter(filter_t * filter) {
    rect_t r;
    rect_array_origin(&layout.filter.rect_array, &r);
    SDL_FillRect(filter_pane, &r, map_sdl_color(filter_pane, layout.filter.bg_color));

    text_render(filter_pane, &layout.filter.name_txt, &filter->color, filter->name);

    graph_update(&(filter->graph_state), filter->output.value);
    graph_render(&(filter->graph_state), layout.graph_filter.line_color);
    SDL_BlitSurface(graph_surface, 0, filter_pane, &layout.graph_filter.rect);
}

static void ui_update_signal(signal_t* signal) {
    rect_t r;
    rect_array_origin(&layout.signal.rect_array, &r);
    SDL_FillRect(signal_pane, &r, map_sdl_color(signal_pane, layout.signal.bg_color));

    SDL_Color signal_c = color_to_SDL(signal->color);
    text_render(signal_pane, &layout.signal.name_txt, &signal_c, signal->name);

    graph_update(&(signal->graph_state), signal->output.value);
    graph_render(&(signal->graph_state), layout.graph_signal.line_color);
    SDL_BlitSurface(graph_surface, 0, signal_pane, &layout.graph_signal.rect);

    for(int i = 0; i < signal->n_params; i++)
    {
        SDL_Color param_name_c = layout.slider.name_color;
        if(&signal->param_states[i] == active_param_source){
            param_name_c = layout.slider.highlight_color;
        }
        slider_render(&signal->parameters[i], &signal->param_states[i], param_name_c);
        rect_array_layout(&layout.signal.sliders_rect_array, i, &r);

        SDL_BlitSurface(slider_surface, 0, signal_pane, &r);
    }
}

static void ui_render_signal_deck(){
    for(int i = 0; i < n_signals; i++) {
        ui_update_signal(&signals[i]);

        rect_t r;
        rect_array_layout(&layout.signal.rect_array, i, &r);
        SDL_BlitSurface(signal_pane, 0, screen, &r);
    }
}

static void ui_render_filter_bank(){
    for(int i = 0; i < n_filters; i++){
        if(!filters[i].display) continue;
        ui_update_filter(&filters[i]);

        rect_t r;
        rect_array_layout(&layout.filter.rect_array, i, &r);
        SDL_BlitSurface(filter_pane, 0, screen, &r);
    }
}

static void ui_update_output(output_strip_t * output_strip){
    rect_t r;
    rect_array_origin(&layout.output.rect_array, &r);
    SDL_FillRect(output_pane, &r, map_sdl_color(output_pane, layout.output.bg_color));
    char buf[16];

    SDL_Color *color = NULL;
    if(output_strip->bus > 0)
        color = &output_strip->color;

    snprintf(buf, 16, "%d %s", output_strip->length, output_strip->id_str);
    text_render(output_pane, &layout.output.name_txt, color, buf);
}

static void ui_render_output_panel(){
    for(int i = 0; i < n_output_strips;i++){
        ui_update_output(&output_strips[i]);

        rect_t r;
        rect_array_layout(&layout.output.rect_array, i, &r);
        SDL_BlitSurface(output_pane, 0, screen, &r);
    }
}

static void ui_update_midi(struct midi_controller * controller){
    rect_t r;
    rect_array_origin(&layout.midi.rect_array, &r);
    SDL_FillRect(midi_pane, &r, map_sdl_color(midi_pane, layout.midi.bg_color));

    if(controller->enabled && controller->short_name)
        text_render(midi_pane, &layout.midi.short_name_txt, 0, controller->short_name);

    if(controller->name)
        text_render(midi_pane, &layout.midi.name_txt, 0, controller->name);
}

static void ui_render_midi_panel(){
    for(int i = 0; i < n_midi_controllers; i++){
        ui_update_midi(&midi_controllers[i]);

        rect_t r;
        rect_array_layout(&layout.midi.rect_array, i, &r);
        SDL_BlitSurface(midi_pane, 0, screen, &r);
    }

    // Draw midi reload button
    ui_draw_button(midi_reload_pane, layout.midi_reload.bg_color, &layout.midi_reload.label_txt, "Refresh MIDI Devices");
    SDL_BlitSurface(midi_reload_pane, 0, screen, &layout.midi_reload.rect);
}

static void ui_update_state_panel(){
    rect_t r;
    for(int i = 0; i < config.state.n_states; i++){
        char buf[16];
        rect_array_layout(&layout.state_save.rect_array, i, &r);
        snprintf(buf, 15, "Save %d", i);
        ui_draw_button(state_save_pane, layout.state_save.bg_color, &layout.state_save.label_txt, buf);
        SDL_BlitSurface(state_save_pane, 0, state_panel_pane, &r);
    }

    for(int i = 0; i < config.state.n_states; i++){
        char buf[16];
        rect_array_layout(&layout.state_load.rect_array, i, &r);
        snprintf(buf, 15, "Load %d", i);
        ui_draw_button(state_load_pane, layout.state_save.bg_color, &layout.state_load.label_txt, buf);
        SDL_BlitSurface(state_load_pane, 0, state_panel_pane, &r);
    }
}

static void ui_update_palette(struct colormap * cm){
    rect_t r;
    rect_array_origin(&layout.palette.rect_array, &r);
    SDL_FillRect(palette_pane, &r, map_sdl_color(palette_pane, layout.palette.bg_color));


    SDL_LockSurface(palette_preview);
    for(int x = 0; x < layout.palette.preview_w; x++){
        color_t color = colormap_color(cm, (float) x / (float) layout.palette.preview_w);
        Uint32 sdlcolor = SDL_MapRGB(palette_preview->format, (uint8_t)roundf(255 * color.r), (uint8_t)roundf(255 * color.g), (uint8_t)roundf(255 * color.b));
        for(int y = 0; y < layout.palette.preview_h; y++){
            ((Uint32 *) palette_preview->pixels)[y * layout.palette.preview_w + x] = sdlcolor;
        }
    }
    SDL_UnlockSurface(palette_preview);
    SDL_BlitSurface(palette_preview, 0, palette_pane, &layout.palette.preview_rect);

    text_render(palette_pane, &layout.palette.name_txt, 0, cm->name);

    if(cm == cm_global){
        SDL_FillRect(palette_pane, &layout.palette.active_rect, map_sdl_color(palette_pane, layout.palette.active_color));
    }else{
        SDL_Color c = layout.palette.active_color;
        rectangleRGBA(palette_pane, 
                layout.palette.active_rect.x,
                layout.palette.active_rect.y,
                layout.palette.active_rect.x + layout.palette.active_rect.w,
                layout.palette.active_rect.y + layout.palette.active_rect.h,
                c.r, c.g, c.b, 255);
    }
}

static void ui_render_palette_panel(){
    rect_t r;
    for(int i = 0; i < n_colormaps; i++){
        ui_update_palette(colormaps[i]);
        rect_array_layout(&layout.palette.rect_array, i, &r);

        SDL_BlitSurface(palette_pane, 0, screen, &r);
    }
}

static void ui_render()
{
    update_ui();

    SDL_FillRect(screen, &layout.window.rect, map_sdl_color(screen, layout.window.bg_color));

    update_master_preview();
    SDL_BlitSurface(master_preview, 0, screen, &layout.master.rect);

    ui_update_slot_deck();
    SDL_BlitSurface(slot_deck_pane, 0, screen, &layout.slot_deck.rect);

    ui_update_audio_panel();
    SDL_BlitSurface(audio_pane, 0, screen, &layout.audio.rect);

    ui_render_filter_bank();
    ui_render_signal_deck();
    ui_render_output_panel();
    ui_render_midi_panel();
    ui_render_palette_panel();

    ui_update_state_panel();
    SDL_BlitSurface(state_panel_pane, 0, screen, &layout.state_panel.rect);

    SDL_Flip(screen);
}

static void get_cursor_in_preview(float* x, float* y)
{
    *x = (float)(mouse_drag_start.x + mouse_drag_delta.x) / layout.slot.preview_w;
    *y = (float)(mouse_drag_start.y + mouse_drag_delta.y) / layout.slot.preview_h;
    if(*x < 0) *x = 0;
    if(*y < 0) *y = 0;
    if(*x > 1) *x = 1;
    if(*y > 1) *y = 1;
}

static void mouse_drag_pattern_ev()
{
    if(!active_preview->pattern) return;

    float x;
    float y;

    get_cursor_in_preview(&x, &y);

    pat_command_t mouse_drag_x = {.index = 0,
                                  .status = STATUS_CHANGE,
                                  .value = x };
    pat_command_t mouse_drag_y = {.index = 1,
                                  .status = STATUS_CHANGE,
                                  .value = y };
    active_preview->pattern->command(active_preview, mouse_drag_x);
    active_preview->pattern->command(active_preview, mouse_drag_y);
}

static void mouse_drop_pattern()
{
    rect_t r;

    for(int i = 0; i < n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        struct xy xy = xy_add(mouse_drag_start, mouse_drag_delta);
        if(xy_in_rect(&xy, &r, 0))
        {
            pat_unload(&slots[i]);
            pat_load(&slots[i],patterns[active_pattern]);
        }
    }
    active_pattern = -1;
}

static void mouse_drop_slot()
{
    rect_t r;

    for(int i = 0; i < n_slots; i++)
    {
        struct xy xy = xy_add(mouse_drag_start, mouse_drag_delta);
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, 0))
        {
            slot_t temp_slot = slots[active_slot];
            slots[active_slot] = slots[i];
            slots[i] = temp_slot;
        }
    }
    active_slot = -1;
}

static void mouse_drop_pattern_ev(struct xy xy)
{
    if(!active_preview->pattern) return;

    float x;
    float y;

    get_cursor_in_preview(&x, &y);

    pat_command_t mouse_drag_x = {.index = 0,
                                  .status = STATUS_STOP,
                                  .value = x };
    pat_command_t mouse_drag_y = {.index = 1,
                                  .status = STATUS_STOP,
                                  .value = y };
    active_preview->pattern->command(active_preview, mouse_drag_x);
    active_preview->pattern->command(active_preview, mouse_drag_y);
    active_preview = 0;
}

static int mouse_down_slot(slot_t* slot, struct xy xy)
{
    rect_t r;
    struct xy offset;
    if(!slots->pattern) return UNHANDLED;

    // See if the click is on the alpha slider
    if(xy_in_rect(&xy, &layout.slot.alpha_rect, &offset))
    {
        PROPAGATE(mouse_down_alpha_slider(&slot->alpha, offset));
    }

    // See if the click is on the palette indicator
    r.x = 0;
    r.w = layout.slot.w;
    r.y = layout.slot.palette_y;
    r.h = layout.slot.palette_size;
    if(xy_in_rect(&xy, &r, &offset)){
        if(active_palette_source == &slot->colormap){
            active_palette_source = NULL;
            slot->colormap = NULL;
        }else{
            active_palette_source = &slots->colormap;
        }
        return HANDLED;
    }

    // See if the click is on the preview 
    if(xy_in_rect(&xy, &layout.slot.preview_rect, &offset))
    {
        pat_command_t mouse_down_x = {.index = 0,
                                      .status = STATUS_START,
                                      .value = (offset.x) / (float) layout.slot.preview_w};
        pat_command_t mouse_down_y = {.index = 1,
                                      .status = STATUS_START,
                                      .value = (offset.y) / (float) layout.slot.preview_h};
        slot->pattern->command(slot, mouse_down_x);
        slot->pattern->command(slot, mouse_down_y);

        mouse_drag_start = offset;
        active_preview = slot;
        mouse_drag_fn_p = &mouse_drag_pattern_ev;
        mouse_drop_fn_p = &mouse_drop_pattern_ev;
        return HANDLED;
    }

    // See if the click is on a parameter slider
    for(int i = 0; i < slot->pattern->n_params; i++)
    {
        rect_array_layout(&layout.slot.sliders_rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset))
        {
            return mouse_down_param_slider(&slot->param_states[i], offset);
        }
    }

    return UNHANDLED;
}

static int mouse_down_slot_deck(struct xy xy) {
    rect_t r;
    struct xy offset;

    // See if click is in a slot
    for(int i=0; i<n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset))
        {
            // If it is, see if that slot wants to handle it
            PROPAGATE(mouse_down_slot(&slots[i], offset));

            // Else, drag the slot
            mouse_drag_start = xy;
            active_slot = i;
            mouse_drop_fn_p = &mouse_drop_slot;
            return HANDLED;
        }
    }

    // See if click is in a pattern
    for(int i=0; i<n_patterns; i++) {
        rect_array_layout(&layout.add_pattern.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset))
        {
            mouse_drag_start = xy;
            active_pattern = i;
            mouse_drop_fn_p = &mouse_drop_pattern;
            return HANDLED;
        }
    }
    return UNHANDLED;
}

static int mouse_down_output(int i, struct xy xy){
    return UNHANDLED;
}

static int mouse_down_midi(int i, struct xy xy){
    //midi_refresh_devices();
    return UNHANDLED;
}

static int mouse_down_signal(int ix, struct xy xy) {
    rect_t r;
    struct xy offset;
    for(int i = 0; i < signals[ix].n_params; i++)
    {
        // See if the click is on a parameter slider
        rect_array_layout(&layout.signal.sliders_rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_down_param_slider(&signals[ix].param_states[i], offset);
        }
    }

    // Was the output clicked
    if(xy_in_rect(&xy, &layout.graph_signal.rect, 0)){
        // Is there something looking for a source?
        if(active_param_source){
            param_state_connect(active_param_source, &signals[ix].output);
            active_param_source = 0;
            return HANDLED;
        }
    }

    return UNHANDLED;
}

static int mouse_down_filter(int i, struct xy xy){
    if(xy_in_rect(&xy, &layout.graph_filter.rect, 0)){
        if(active_param_source){
            param_state_connect(active_param_source, &filters[i].output);
            active_param_source = 0;
        }
        return 1;
    }
    return 0;
}

static int mouse_down_audio(struct xy xy){
    rect_t r;
    struct xy offset;

    for(int i = 0; i < N_WF_BINS; i++){
        rect_array_layout(&layout.audio.bins_rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            if(active_param_source){
                param_state_connect(active_param_source, &waveform_bins[i].output);
                active_param_source = 0;
                return HANDLED;
            }
            return UNHANDLED;
        }
    }

    if(xy_in_rect(&xy, &layout.waveform.rect, &offset)){
        timebase_tap(config.timebase.beat_click_alpha);
        return HANDLED;
    }

    if(xy_in_rect(&xy, &layout.audio.ball_rect, &offset)){
        printf("ball clicked\n");
        timebase_align();
        return HANDLED;
    }

    if(xy_in_rect(&xy, &layout.audio.auto_rect, &offset)){
        if(timebase_source == TB_AUTOMATIC)
            timebase_source = TB_MANUAL;
        else 
            timebase_source = TB_AUTOMATIC;
        return HANDLED;
    }

    return UNHANDLED;
}

static int mouse_down_state_save(int i, struct xy xy){
    char filename[1024];
    snprintf(filename, 1023, config.state.path_format, i);
    if(state_save(filename)) printf("Error saving state to '%s'\n", filename);
    return HANDLED;
}

static int mouse_down_state_load(int i, struct xy xy){
    char filename[1024];
    snprintf(filename, 1023, config.state.path_format, i);
    if(state_load(filename)) printf("Error loading state from '%s'\n", filename);
    return HANDLED;
}

static int mouse_down_state_panel(struct xy xy){
    rect_t r;
    struct xy offset;

    // See if click is on a save state button
    for(int i = 0; i < config.state.n_states; i++){
        rect_array_layout(&layout.state_save.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            PROPAGATE(mouse_down_state_save(i, offset));
            break;
        }
    }

    // See if click is on a load state button
    for(int i = 0; i < config.state.n_states; i++){
        rect_array_layout(&layout.state_load.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            PROPAGATE(mouse_down_state_load(i, offset));
            break;
        }
    }

    return UNHANDLED;
}

static int mouse_down_palette(int i, struct xy xy){
    struct xy offset;
    int rc = UNHANDLED;

    if(active_palette_source){
        *active_palette_source = colormaps[i];
        active_palette_source = NULL;
        rc = HANDLED;
    }

    if(i == 0) return rc;

    if(xy_in_rect(&xy, &layout.palette.active_rect, &offset)){
        colormap_set_global(colormaps[i]);
        rc = HANDLED;
    }

    if(colormaps[i] == cm_global && xy_in_rect(&xy, &layout.palette.preview_rect, &offset)){
        colormap_set_mono((float) offset.x / (float) layout.palette.w);
        rc = HANDLED;
    }
    return UNHANDLED;
}

static int mouse_down(struct xy xy) {
    struct xy offset;
    rect_t r;
    
    // See if click is in master pane
    if(xy_in_rect(&xy, &layout.master.rect, &offset)){
        return UNHANDLED;
    }

    if(xy_in_rect(&xy, &layout.slot_deck.rect, &offset)){
        PROPAGATE(mouse_down_slot_deck(offset));
    }

    // See if click is in audio pane
    if(xy_in_rect(&xy, &layout.audio.rect, &offset)){
        PROPAGATE(mouse_down_audio(offset));
    }

    // See if click is in filter
    for(int i = 0; i < n_filters; i++){
        rect_array_layout(&layout.filter.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            PROPAGATE(mouse_down_filter(i, offset));
            break;
        }
    }

    // See if click is in an signal
    for(int i = 0; i < n_signals; i++){
        rect_array_layout(&layout.signal.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            PROPAGATE(mouse_down_signal(i, offset));
            break;
        }
    }

    // See if click is in output
    for(int i =  0; i < n_output_strips; i++){
        rect_array_layout(&layout.output.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            PROPAGATE(mouse_down_output(i, offset));
            break;
        }
    }

    // See if click is in midi
    for(int i =  0; i < n_midi_controllers; i++){
        rect_array_layout(&layout.midi.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            PROPAGATE(mouse_down_midi(i, offset));
            break;
        }
    }

    // See if click is on midi reload button
    if(xy_in_rect(&xy, &layout.midi_reload.rect, &offset)){
        midi_refresh_devices();
        return HANDLED;
    }

    if(xy_in_rect(&xy, &layout.state_panel.rect, &offset)){
        PROPAGATE(mouse_down_state_panel(offset));
    }

    // See if click is on a palette
    for(int i = 0; i < n_colormaps; i++){
        rect_array_layout(&layout.palette.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_down_palette(i, offset);
        }
    }

    // Otherwise, do not handle click
    return UNHANDLED;
}

static void ui_poll()
{
    SDL_Event e;
    struct xy xy;
    midi_command_event_data_t* event_data;
    while(SDL_PollEvent(&e)) 
    {
        // Not always valid depending on event type
        xy.x = e.button.x;
        xy.y = e.button.y;
        switch(e.type)
        {
            case SDL_QUIT:
                ui_running = 0;
                if(ui_done_fn) (*ui_done_fn)();
                break;
            case SDL_MOUSEBUTTONDOWN:
                // If there's an active param source, cancel it after the click
                if(active_param_source)
                {
                    mouse_down(xy);
                    active_param_source = NULL;
                }
                else
                {
                    mouse_down(xy);
                }
                mouse_is_down = 1;
                mouse_drag_start_abs = xy;
                mouse_drag_delta.x = 0;
                mouse_drag_delta.y = 0;
                break;
            case SDL_MOUSEMOTION:
                if(mouse_is_down)
                {
                    mouse_drag_delta  = xy_sub(xy, mouse_drag_start_abs);
                    if(mouse_drag_fn_p) (*mouse_drag_fn_p)();
                }
                break;
            case SDL_MOUSEBUTTONUP:
                mouse_drag_delta = xy_sub(xy, mouse_drag_start_abs);
                if(mouse_drop_fn_p)
                {
                    (*mouse_drop_fn_p)();
                }
                mouse_drag_fn_p = 0;
                mouse_drop_fn_p = 0;
                mouse_is_down = 0;
                break;
            case SDL_MIDI_COMMAND_EVENT:
                event_data = (midi_command_event_data_t*) e.user.data1;
                if(event_data->slot->pattern)
                {
                    event_data->slot->pattern->command(event_data->slot, event_data->command);
                }
                free(event_data);
                break;
        }
    }
}

static int ui_run(void* args)
{
    FPSmanager fps_manager;

    ui_init();

    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 100);
    stat_fps = 100;

    unsigned int last_tick = SDL_GetTicks();
    while(ui_running)
    {
        ui_render();
        ui_poll();
        SDL_framerateDelay(&fps_manager);

        // No @ervanalb, `SDL_getFramerate(...)` just returns whatever you *set* the framerate to (100).
        //stat_fps = SDL_getFramerate(&fps_manager);
        
        // Calculate fps and simple LPF
        stat_fps = 0.8 * stat_fps + 0.2 * (1000. / (SDL_GetTicks() - last_tick));
        last_tick = SDL_GetTicks();
    }
    return 0;
}

void ui_start(void (*ui_done)())
{
    ui_running = 1;
    ui_done_fn = ui_done;

    ui_thread = SDL_CreateThread(&ui_run, 0);
    if(!ui_thread) FAIL("Could not create UI thread: %s\n",SDL_GetError());
}

void ui_stop()
{
    ui_running = 0;

    SDL_WaitThread(ui_thread, 0);
}


