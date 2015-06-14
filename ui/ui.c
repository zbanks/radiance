#include <math.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

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
#include "core/state.h"
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
    X(slot_pane, layout.slot) \
    X(pattern_pane, layout.add_pattern) \
    X(signal_pane, layout.signal) \
    X(filter_pane, layout.filter) \
    X(output_pane, layout.output) \
    X(midi_pane, layout.midi) \
    X(palette_pane, layout.palette) \
    X(palette_preview, layout.palette.preview_rect) \
    X(state_save_pane, layout.state_save) \
    X(state_load_pane, layout.state_load)

#define X(s, l) \
    static SDL_Surface* s;
SURFACES
#undef X

static SDL_Surface * screen;

static int mouse_down;
static struct xy mouse_drag_start;

void (*mouse_drag_fn_p)(struct xy);
void (*mouse_drop_fn_p)(struct xy);

static struct
{
    int index;
    struct xy dxy;
} active_pattern;

static struct
{
    slot_t * slot;
    struct xy dxy;
} active_slot;

static struct
{
    slot_t * slot;
    struct xy dxy;
} active_preview;

param_state_t * active_param_source;
struct colormap ** active_palette_source;

static size_t master_pixels;
static float * master_xs;
static float * master_ys;
static color_t * master_frame;

void ui_init()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        FAIL("SDL_Init Error: %s\n", SDL_GetError());
    }

    if (TTF_Init())
    {
        FAIL("TTF_Init Error: %s\n", SDL_GetError());
    }

    screen = SDL_SetVideoMode(layout.window.w, layout.window.h, 0, SDL_DOUBLEBUF);

    if (!screen) FAIL("SDL_SetVideoMode Error: %s\n", SDL_GetError());

#define X(s, l) \
    s = SDL_CreateRGBSurface(0, l.w, l.h, 32, 0, 0, 0, 0); \
    if(!s) FAIL("SDL_CreateRGBSurface Error: %s\n", SDL_GetError());
SURFACES
#undef X

    slider_init();
    graph_init();
    ui_waveform_init();

    mouse_down = 0;
    mouse_drag_fn_p = 0;
    mouse_drop_fn_p = 0;
    active_pattern.index = -1;
    active_slot.slot = 0;
    active_preview.slot = 0;

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

static void update_master_preview()
{
    SDL_LockSurface(master_preview);

    render_composite_frame(slots, master_xs, master_ys, master_pixels, master_frame);
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

    for(int i=0; i<n_output_strips; i++)
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

static void update_pattern_preview(slot_t* slot)
{
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
            color_t pixel = (*slot->pattern->render)(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.slot.preview_w * y] = SDL_MapRGB(
                pattern_preview->format,
                (uint8_t)roundf(255 * (pixel.r * pixel.a + (1.0 - pixel.a) * bg_shade)),
                (uint8_t)roundf(255 * (pixel.g * pixel.a + (1.0 - pixel.a) * bg_shade)),
                (uint8_t)roundf(255 * (pixel.b * pixel.a + (1.0 - pixel.a) * bg_shade)));
        }
    }

    SDL_UnlockSurface(pattern_preview);
}

static void ui_update_audio(){
    rect_t r;
    rect_origin(&layout.audio.rect, &r);
    SDL_FillRect(audio_pane, &r, SDL_MapRGB(audio_pane->format, 20, 20, 20));

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
    filledCircleRGBA(audio_pane, x, y, layout.audio.ball_r, 255, 10, 10, 255);
    hlineRGBA(audio_pane, layout.audio.ball_x, layout.audio.ball_x + layout.audio.ball_w, layout.audio.ball_y + layout.audio.ball_h, 255, 255, 255, 255);


    char buf[16];
    snprintf(buf, 16, "bpm: %.2f", timebase_get_bpm());
    text_render(audio_pane, &layout.audio.bpm_txt, 0, buf);

    snprintf(buf, 16, "fps: %d", stat_fps);
    text_render(audio_pane, &layout.audio.fps_txt, 0, buf);

    snprintf(buf, 16, "ops: %d", stat_ops);
    text_render(audio_pane, &layout.audio.ops_txt, 0, buf);

    snprintf(buf, 16, "%lu", ((time % 4000) / 1000) + 1);
    text_render(audio_pane, &layout.audio.beat_txt, 0, buf);

    for(int i = 0; i < N_WF_BINS; i++){
        rect_array_layout(&layout.audio.bins_rect_array, i, &r);
        SDL_FillRect(audio_pane, &r, SDL_MapRGB(audio_pane->format,
                    waveform_bins[i].color.r,
                    waveform_bins[i].color.g,
                    waveform_bins[i].color.b));
    }

    if(timebase_source == TB_AUTOMATIC)
        boxRGBA(audio_pane, layout.audio.auto_x, layout.audio.auto_y, layout.audio.auto_x + layout.audio.auto_w, layout.audio.auto_y + layout.audio.auto_h, 10, 255, 10, 255);
    else
        boxRGBA(audio_pane, layout.audio.auto_x, layout.audio.auto_y, layout.audio.auto_x + layout.audio.auto_w, layout.audio.auto_y + layout.audio.auto_h, 255, 10, 10, 255);
}

static void ui_update_output(output_strip_t * output_strip){
    rect_t r;
    rect_array_origin(&layout.output.rect_array, &r);
    SDL_FillRect(output_pane, &r, SDL_MapRGB(output_pane->format, 20, 20, 20));
    char buf[16];

    SDL_Color color = {150, 150, 150, 255};

    if(output_strip->bus > 0)
        color = output_strip->color;

    snprintf(buf, 16, "%d %s", output_strip->length, output_strip->id_str);
    text_render(output_pane, &layout.output.name_txt, &color, buf);
}

static void ui_update_midi(struct midi_controller * controller){
    rect_t r;
    rect_array_origin(&layout.midi.rect_array, &r);
    SDL_FillRect(midi_pane, &r, SDL_MapRGB(midi_pane->format, 20, 20, 20));

    SDL_Color color = {150, 150, 150, 255};

    if(controller->enabled)
        color = (SDL_Color) {0, 255, 30, 255};

    if(controller->short_name)
        text_render(midi_pane, &layout.midi.name_txt, &color, controller->short_name);
    else if(controller->name)
        text_render(midi_pane, &layout.midi.name_txt, &color, controller->name);
}

static void ui_update_slot(slot_t* slot)
{
    rect_t r;
    SDL_Color param_name_c = {255, 255, 255, 255};
    SDL_Color highlight_c = {255, 200, 0, 255};
    rect_array_origin(&layout.slot.rect_array, &r);
    SDL_FillRect(slot_pane, &r, SDL_MapRGB(slot_pane->format, 20, 20, 20));

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
            text_render(slot_pane, &layout.slot.palette_txt, &highlight_c, palette_str);
        else
            text_render(slot_pane, &layout.slot.palette_txt, 0, palette_str);

        slider_render_alpha(&slot->alpha);
        SDL_BlitSurface(alpha_slider_surface, 0, slot_pane, &layout.slot.alpha_rect);

        for(int i = 0; i < slot->pattern->n_params; i++)
        {
            if(&slot->param_states[i] == active_param_source){
                slider_render(&slot->pattern->parameters[i], &slot->param_states[i], highlight_c);
            }else{
                slider_render(&slot->pattern->parameters[i], &slot->param_states[i], param_name_c);
            }
            rect_array_layout(&layout.slot.sliders_rect_array, i, &r);

            SDL_BlitSurface(slider_surface, 0, slot_pane, &r);
        }
        
    }
}

static void ui_update_filter(filter_t * filter)
{
    rect_t r;
    SDL_Color white = {255, 255, 255, 255};

    rect_array_origin(&layout.filter.rect_array, &r);
    SDL_FillRect(filter_pane, &r, SDL_MapRGB(filter_pane->format, 20, 20, 20));

    text_render(filter_pane, &layout.filter.name_txt, &filter->color, filter->name);

    graph_update(&(filter->graph_state), filter->output.value);
    graph_render(&(filter->graph_state), white);
    SDL_BlitSurface(graph_surface, 0, filter_pane, &layout.graph_filter.rect);
}

static void ui_update_pattern(pattern_t* pattern)
{
    rect_t r;
    rect_array_origin(&layout.add_pattern.rect_array, &r);
    SDL_FillRect(pattern_pane, &r, SDL_MapRGB(pattern_pane->format, 20, 20, 20));

    text_render(pattern_pane, &layout.add_pattern.name_txt, 0, pattern->name);
}

static void ui_update_signal(signal_t* signal)
{
    rect_t r;
    SDL_Color param_name_c;
    SDL_Color signal_c;
    rect_array_origin(&layout.signal.rect_array, &r);
    SDL_FillRect(signal_pane, &r, SDL_MapRGB(signal_pane->format, 20, 20, 20));

    signal_c = color_to_SDL(signal->color);
    text_render(signal_pane, &layout.signal.name_txt, &signal_c, signal->name);

    graph_update(&(signal->graph_state), signal->output.value);
    SDL_Color white = {255, 255, 255, 255};
    graph_render(&(signal->graph_state), white);
    SDL_BlitSurface(graph_surface, 0, signal_pane, &layout.graph_signal.rect);

    for(int i = 0; i < signal->n_params; i++)
    {
        if(&signal->param_states[i] == active_param_source){
            //param_name_c = {255, 200, 0};
            param_name_c.r = 255;
            param_name_c.g = 200;
            param_name_c.b =   0;
        }else{
            //param_name_c = {255, 255, 255};
            param_name_c.r = 255;
            param_name_c.g = 255;
            param_name_c.b = 255;
        }
        slider_render(&signal->parameters[i], &signal->param_states[i], param_name_c);
        rect_array_layout(&layout.signal.sliders_rect_array, i, &r);

        SDL_BlitSurface(slider_surface, 0, signal_pane, &r);
    }
}

static void ui_update_palette(struct colormap * cm){
    rect_t r;
    rect_array_origin(&layout.palette.rect_array, &r);
    SDL_FillRect(palette_pane, &r, SDL_MapRGB(palette_pane->format, 20, 20, 20));


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
        SDL_FillRect(palette_pane, &layout.palette.active_rect, SDL_MapRGB(palette_pane->format, 255, 255, 255));
    }else{
        rectangleRGBA(palette_pane, 
                layout.palette.active_rect.x,
                layout.palette.active_rect.y,
                layout.palette.active_rect.x + layout.palette.active_rect.w,
                layout.palette.active_rect.y + layout.palette.active_rect.h,
                255, 255, 255, 255);
    }
}

static void ui_draw_button(SDL_Surface * surface, struct txt * label_fmt, const char * label){
    rect_t r = {.x = 0, .y = 0, .w = surface->w, .h = surface->h};
    SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, 20, 20, 20));
    text_render(surface, label_fmt, 0, label);
}

void ui_render()
{
    rect_t r;
    SDL_FillRect(screen, &layout.window.rect, SDL_MapRGB(screen->format, 0, 0, 0));

    update_master_preview();
    SDL_BlitSurface(master_preview, 0, screen, &layout.master.rect);

    ui_update_audio();
    SDL_BlitSurface(audio_pane, 0, screen, &layout.audio.rect);

    for(int i = 0; i < n_signals; i++)
    {
        ui_update_signal(&signals[i]);
        rect_array_layout(&layout.signal.rect_array, i, &r);
        SDL_BlitSurface(signal_pane, 0, screen, &r);
    }

    for(int i=0; i<n_slots; i++)
    {
        if(slots[i].pattern)
        {
            ui_update_slot(&slots[i]);
            rect_array_layout(&layout.slot.rect_array, i, &r);

            if(active_slot.slot == &slots[i]) {
                rect_shift(&r, &active_slot.dxy);
            }

            SDL_BlitSurface(slot_pane, 0, screen, &r);
        }
    }

    rect_array_layout(&layout.slot.rect_array, 2, &r);
    vlineRGBA(screen, r.x - 4, r.y, r.y + r.h, 255, 10, 30, 255);
    vlineRGBA(screen, r.x - 5, r.y-1, r.y + r.h +1, 255, 10, 30, 255);
    vlineRGBA(screen, r.x - 6, r.y, r.y + r.h, 255, 10, 30, 255);
    rect_array_layout(&layout.slot.rect_array, 4, &r);
    vlineRGBA(screen, r.x - 4, r.y, r.y + r.h, 200, 30, 255, 255);
    vlineRGBA(screen, r.x - 5, r.y-1, r.y + r.h +1, 200, 30, 255, 255);
    vlineRGBA(screen, r.x - 6, r.y, r.y + r.h, 200, 30, 255, 255);
    rect_array_layout(&layout.slot.rect_array, 6, &r);
    vlineRGBA(screen, r.x - 4, r.y, r.y + r.h, 10, 255, 20, 255);
    vlineRGBA(screen, r.x - 5, r.y-1, r.y + r.h +1, 10, 255, 20, 255);
    vlineRGBA(screen, r.x - 6, r.y, r.y + r.h, 10, 255, 20, 255);

    for(int i=0; i<n_patterns; i++)
    {
        ui_update_pattern(patterns[i]);
        rect_array_layout(&layout.add_pattern.rect_array, i, &r);

        if(active_pattern.index == i) {
            rect_shift(&r, &active_pattern.dxy);
        }

        SDL_BlitSurface(pattern_pane, 0, screen, &r);
    }

    for(int i = 0; i < n_filters; i++){
        if(!filters[i].display) continue;
        ui_update_filter(&filters[i]);
        rect_array_layout(&layout.filter.rect_array, i, &r);

        SDL_BlitSurface(filter_pane, 0, screen, &r);
    }

    for(int i = 0; i < n_output_strips;i++){
        ui_update_output(&output_strips[i]);
        rect_array_layout(&layout.output.rect_array, i, &r);

        SDL_BlitSurface(output_pane, 0, screen, &r);
    }

    for(int i = 0; i < n_midi_controllers; i++){
        ui_update_midi(&midi_controllers[i]);
        rect_array_layout(&layout.midi.rect_array, i, &r);
        
        SDL_BlitSurface(midi_pane, 0, screen, &r);
    }

    for(int i = 0; i < config.state.n_states; i++){
        char buf[16];
        rect_array_layout(&layout.state_save.rect_array, i, &r);
        snprintf(buf, 15, "Save %d", i);
        ui_draw_button(state_save_pane, &layout.state_save.label_txt, buf);
        SDL_BlitSurface(state_save_pane, 0, screen, &r);
    }

    for(int i = 0; i < config.state.n_states; i++){
        char buf[16];
        rect_array_layout(&layout.state_load.rect_array, i, &r);
        snprintf(buf, 15, "Load %d", i);
        ui_draw_button(state_load_pane, &layout.state_load.label_txt, buf);
        SDL_BlitSurface(state_load_pane, 0, screen, &r);
    }

    for(int i = 0; i < n_colormaps; i++){
        ui_update_palette(colormaps[i]);
        rect_array_layout(&layout.palette.rect_array, i, &r);

        SDL_BlitSurface(palette_pane, 0, screen, &r);
    }

    SDL_Flip(screen);
}

static void mouse_drag_pattern(struct xy xy)
{
    active_pattern.dxy = xy; 
}

static void mouse_drag_slot(struct xy xy)
{
    active_slot.dxy = xy; 
}

static void mouse_drag_pattern_ev(struct xy xy){
    if(!active_preview.slot->pattern) return;
    struct xy offset;
    xy = xy_add(xy, active_preview.dxy);
    if(xy_in_rect(&xy, &layout.slot.preview_rect, &offset)){
        static struct pat_event mouse_drag_x = {.source = PATSRC_MOUSE_X, .event = PATEV_MIDDLE};
        static struct pat_event mouse_drag_y = {.source = PATSRC_MOUSE_Y, .event = PATEV_MIDDLE};
        active_preview.slot->pattern->event(active_preview.slot, mouse_drag_x,
                -1.0 + 2.0 * (offset.x) / (float) layout.slot.preview_w);
        active_preview.slot->pattern->event(active_preview.slot, mouse_drag_y,
                -1.0 + 2.0 * (offset.y) / (float) layout.slot.preview_h);
    }
}

static void mouse_drop_pattern(struct xy unused)
{
    UNUSED(unused);
    rect_t r;
    struct xy xy;
    xy = xy_add(active_pattern.dxy, mouse_drag_start);

    for(int i = 0; i < n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, 0)){
            pat_unload(&slots[i]);
            pat_load(&slots[i],patterns[active_pattern.index]);
        }
    }
    active_pattern.index = -1;
}

static void mouse_drop_slot(struct xy unused)
{
    UNUSED(unused);
    rect_t r;
    struct xy xy;
    xy = xy_add(active_slot.dxy, mouse_drag_start);
    int max_x = 0; // XXX This breaks in different layouts

    for(int i = 0; i < n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, 0)){
            slot_t temp_slot = *active_slot.slot;
            *active_slot.slot = slots[i];
            slots[i] = temp_slot;
        }
        max_x = max_x > (r.x + r.w) ? max_x : (r.x + r.w);
    }
    if(xy.x > max_x){
        pat_unload(active_slot.slot);
    }
    active_slot.slot = 0;
}

static void mouse_drop_pattern_ev(struct xy xy){
    if(!active_preview.slot->pattern) return;
    struct xy offset;
    xy = xy_add(xy, active_preview.dxy);
    float x = NAN;
    float y = NAN;
    if(xy_in_rect(&xy, &layout.slot.preview_rect, &offset)){
        x = -1.0 + 2.0 * (offset.x) / (float) layout.slot.preview_w;
        y = -1.0 + 2.0 * (offset.y) / (float) layout.slot.preview_h;
    }
    static struct pat_event mouse_up_x = {.source = PATSRC_MOUSE_X, .event = PATEV_END};
    static struct pat_event mouse_up_y = {.source = PATSRC_MOUSE_Y, .event = PATEV_END};
    active_preview.slot->pattern->event(active_preview.slot, mouse_up_x, x);
    active_preview.slot->pattern->event(active_preview.slot, mouse_up_y, y);
}

static int mouse_click_slot(int index, struct xy xy)
{
    rect_t r;
    struct xy offset;
    if(!slots[index].pattern) return 0;

    // See if the click is on the alpha slider
    if(xy_in_rect(&xy, &layout.slot.alpha_rect, &offset)){
        return !!mouse_click_alpha_slider(&slots[index].alpha, offset);
    }

    // See if the click is on the palette indicator
    r.x = 0;
    r.w = layout.slot.w;
    r.y = layout.slot.palette_y;
    r.h = layout.slot.palette_size;
    if(xy_in_rect(&xy, &r, &offset)){
        if(active_palette_source == &slots[index].colormap){
            active_palette_source = NULL;
            slots[index].colormap = NULL;
        }else{
            active_palette_source = &slots[index].colormap;
        }
        return 1;
    }

    // See if the click is on the preview 
    if(xy_in_rect(&xy, &layout.slot.preview_rect, &offset)){
        static struct pat_event mouse_down_x = {.source = PATSRC_MOUSE_X, .event = PATEV_START};
        static struct pat_event mouse_down_y = {.source = PATSRC_MOUSE_Y, .event = PATEV_START};
        slots[index].pattern->event(&slots[index], mouse_down_x,
                -1.0 + 2.0 * (offset.x) / (float) layout.slot.preview_w);
        slots[index].pattern->event(&slots[index], mouse_down_y,
                -1.0 + 2.0 * (offset.y) / (float) layout.slot.preview_h);
        active_preview.slot = &slots[index];
        memcpy(&active_preview.dxy, &xy, sizeof(struct xy));
        mouse_drag_fn_p = &mouse_drag_pattern_ev;
        mouse_drop_fn_p = &mouse_drop_pattern_ev;
        return 1; 
    }

    // See if the click is on a parameter slider
    for(int i = 0; i < slots[index].pattern->n_params; i++)
    {
        rect_array_layout(&layout.slot.sliders_rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&slots[index].param_states[i], offset);
        }
    }

    // Else, drag the slot
    active_slot.slot = &slots[index];
    active_slot.dxy = (struct xy) {0, 0};

    mouse_drag_fn_p = &mouse_drag_slot;
    mouse_drop_fn_p = &mouse_drop_slot;
    return 1;

}

static int mouse_click_output(int index, struct xy xy){
    UNUSED(index);
    UNUSED(xy);
    return 0;
}

static int mouse_click_midi(int index, struct xy xy){
    UNUSED(index);
    UNUSED(xy);
    //midi_refresh_devices();
    return 0;
}

static int mouse_click_signal(int index, struct xy xy)
{
    rect_t r;
    struct xy offset;
    for(int i = 0; i < signals[index].n_params; i++)
    {
        // See if the click is on a parameter slider
        rect_array_layout(&layout.signal.sliders_rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&signals[index].param_states[i], offset);
        }
    }

    // Was the output clicked
    if(xy_in_rect(&xy, &layout.graph_signal.rect, 0)){
        // Is there something looking for a source?
        if(active_param_source){
            param_state_connect(active_param_source, &signals[index].output);
            active_param_source = 0;
        }
    }

    return 0;
}

static int mouse_click_filter(int index, struct xy xy){
    if(xy_in_rect(&xy, &layout.graph_filter.rect, 0)){
        if(active_param_source){
            param_state_connect(active_param_source, &filters[index].output);
            active_param_source = 0;
        }
        return 1;
    }
    return 0;
}

static int mouse_click_audio(struct xy xy){
    rect_t r;
    struct xy offset;

    for(int i = 0; i < N_WF_BINS; i++){
        rect_array_layout(&layout.audio.bins_rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            if(active_param_source){
                param_state_connect(active_param_source, &waveform_bins[i].output);
                active_param_source = 0;
            }
            return 1;
        }
    }

    if(xy_in_rect(&xy, &layout.waveform.rect, &offset)){
        timebase_tap(config.timebase.beat_click_alpha);
    }

    if(xy_in_rect(&xy, &layout.audio.ball_rect, &offset)){
        printf("ball clicked\n");
        timebase_align();
    }

    if(xy_in_rect(&xy, &layout.audio.auto_rect, &offset)){
        if(timebase_source == TB_AUTOMATIC)
            timebase_source = TB_MANUAL;
        else 
            timebase_source = TB_AUTOMATIC;
    }

    return 1;
}

static int mouse_click_state_save(int index, struct xy xy){
    UNUSED(xy);
    char filename[1024];
    snprintf(filename, 1023, config.state.path_format, index);
    if(state_save(filename)) printf("Error saving state to '%s'\n", filename);
    return 1;
}

static int mouse_click_state_load(int index, struct xy xy){
    UNUSED(xy);
    char filename[1024];
    snprintf(filename, 1023, config.state.path_format, index);
    if(state_load(filename)) printf("Error loading state from '%s'\n", filename);
    return 1;
}

static int mouse_click_palette(int index, struct xy xy){
    struct xy offset;
    if(active_palette_source){
        *active_palette_source = colormaps[index];
        active_palette_source = NULL;
    }

    if(index == 0) return 1;

    if(xy_in_rect(&xy, &layout.palette.active_rect, &offset)){
        colormap_set_global(colormaps[index]);
    }

    if(colormaps[index] == cm_global && xy_in_rect(&xy, &layout.palette.preview_rect, &offset)){
        colormap_set_mono((float) offset.x / (float) layout.palette.w);
    }
    return 1;
}

static int mouse_click(struct xy xy)
{
    struct xy offset;
    rect_t r;
    
    // See if click is in master pane
    if(xy_in_rect(&xy, &layout.master.rect, &offset)){
        return 1;
    }

    // See if click is in audio pane
    if(xy_in_rect(&xy, &layout.audio.rect, &offset)){
        return mouse_click_audio(offset);
    }


    // See if click is in a slot
    for(int i=0; i<n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            // If it is, see if that slot wants to handle it
            return mouse_click_slot(i, offset);
        }
    }

    // See if click is in a pattern
    for(int i=0; i<n_patterns; i++)
    {
        rect_array_layout(&layout.add_pattern.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            active_pattern.index = i;
            active_pattern.dxy = (struct xy) {0,0};
            mouse_drag_fn_p = &mouse_drag_pattern;
            mouse_drop_fn_p = &mouse_drop_pattern;
            return 1;
        }
    }

    // See if click is in an signal
    for(int i = 0; i < n_signals; i++){
        rect_array_layout(&layout.signal.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_click_signal(i, offset);
        }
    }

    // See if click is in filter
    for(int i = 0; i < n_filters; i++){
        rect_array_layout(&layout.filter.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_click_filter(i, offset);
        }
    }

    // See if click is in output
    for(int i =  0; i < n_output_strips; i++){
        rect_array_layout(&layout.output.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_click_output(i, offset);
        }
    }

    // See if click is in midi
    for(int i =  0; i < n_midi_controllers; i++){
        rect_array_layout(&layout.midi.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_click_midi(i, offset);
        }
    }

    // See if click is on a save state button
    for(int i = 0; i < config.state.n_states; i++){
        rect_array_layout(&layout.state_save.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_click_state_save(i, offset);
        }
    }

    // See if click is on a load state button
    for(int i = 0; i < config.state.n_states; i++){
        rect_array_layout(&layout.state_load.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_click_state_load(i, offset);
        }
    }

    // See if click is on a palette
    for(int i = 0; i < n_colormaps; i++){
        rect_array_layout(&layout.palette.rect_array, i, &r);
        if(xy_in_rect(&xy, &r, &offset)){
            return mouse_click_palette(i, offset);
        }
    }

    // Otherwise, do not handle click
    // TEMP: treat click as beat tap
    //timebase_tap(0.8);
    return 1;
    //return 0;
}

int ui_poll()
{
    SDL_Event e;
    struct xy xy;
    while(SDL_PollEvent(&e)) 
    {
        // Not always valid depending on event type
        xy.x = e.button.x;
        xy.y = e.button.y;
        switch(e.type)
        {
            case SDL_QUIT:
                return 0;
            case SDL_MOUSEBUTTONDOWN:
                // If there's an active param source, cancel it after the click
                if(active_param_source){
                    mouse_click(xy);
                    active_param_source = NULL;
                }else{
                    mouse_click(xy);
                }
                mouse_down = 1;
                mouse_drag_start = xy;
                break;
            case SDL_MOUSEMOTION:
                if(mouse_down && mouse_drag_fn_p){
                    xy  = xy_sub(xy, mouse_drag_start);
                    (*mouse_drag_fn_p)(xy);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(mouse_drop_fn_p){
                    xy = xy_sub(xy, mouse_drag_start);
                    (*mouse_drop_fn_p)(xy);
                }
                mouse_drag_fn_p = 0;
                mouse_drop_fn_p = 0;
                mouse_down = 0;
                break;
            case SDL_MIDI_SLOT_EVENT:
                xy.x = 0; //XXX
                struct midi_slot_event * slot_event = (struct midi_slot_event *) e.user.data1;
                float * value = (float *) e.user.data2;
                if(slot_event->slot->pattern){
                    slot_event->slot->pattern->event(slot_event->slot, slot_event->event, *value);
                }
                free(value);
                break;
        }
    }
    return 1;
}

