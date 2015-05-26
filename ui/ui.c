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
#include "core/err.h"
#include "core/parameter.h"
#include "core/slot.h"
#include "filters/filter.h"
#include "waveform/waveform.h"
#include "hits/hit.h"
#include "midi/midi.h"
#include "midi/layout.h"
#include "midi/controllers.h"
#include "output/slice.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "timebase/timebase.h"

#define SURFACES \
    X(master_preview, layout.master) \
    X(pattern_preview, layout.slot.preview_rect) \
    X(audio_pane, layout.audio) \
    X(slot_pane, layout.slot) \
    X(hit_slot_pane, layout.hit_slot) \
    X(pattern_pane, layout.add_pattern) \
    X(hit_pane, layout.add_hit) \
    X(signal_pane, layout.signal) \
    X(filter_pane, layout.filter) \
    X(output_pane, layout.output) \
    X(midi_pane, layout.midi) 

#define X(s, l) \
    static SDL_Surface* s;
SURFACES
#undef X

static SDL_Surface * screen;
static TTF_Font* pattern_font;
static TTF_Font* signal_font;
static TTF_Font* filter_font;

static int mouse_down;
static struct xy mouse_drag_start;

void (*mouse_drag_fn_p)(struct xy);
void (*mouse_drop_fn_p)();

static struct
{
    int index;
    struct xy dxy;
} active_pattern;

static struct
{
    int index;
    struct xy dxy;
} active_hit;

static struct
{
    slot_t * slot;
    struct xy dxy;
    int is_pattern;
} active_slot;

static struct active_hit * active_active_hit;

param_state_t * active_param_source;

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

    pattern_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 20);
    if(!pattern_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    signal_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 20);
    if(!signal_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    filter_font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 14);
    if(!filter_font) FAIL("TTF_OpenFont Error: %s\n", SDL_GetError());

    slider_init();
    graph_init();
    ui_waveform_init();

    mouse_down = 0;
    mouse_drag_fn_p = 0;
    mouse_drop_fn_p = 0;
    active_pattern.index = -1;
    active_slot.slot = 0;
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

    TTF_CloseFont(pattern_font);
    TTF_CloseFont(signal_font);
    TTF_CloseFont(filter_font);

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

    for(int x=0;x<layout.master.w;x++)
    {
        for(int y=0;y<layout.master.h;y++)
        {
            float xf = ((float)x / (layout.master.w - 1)) * 2 - 1;
            float yf = ((float)y / (layout.master.h - 1)) * 2 - 1;
            color_t pixel = render_composite(xf, yf);
            ((uint32_t*)(master_preview->pixels))[x + layout.master.w * y] = SDL_MapRGB(
                master_preview->format,
                (uint8_t)roundf(255 * pixel.r),
                (uint8_t)roundf(255 * pixel.g),
                (uint8_t)roundf(255 * pixel.b));
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
            float xf = ((float)x / (layout.slot.preview_w - 1)) * 2 - 1;
            float yf = ((float)y / (layout.slot.preview_h - 1)) * 2 - 1;
            color_t pixel = (*slot->pattern->render)(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.slot.preview_w * y] = SDL_MapRGB(
                pattern_preview->format,
                (uint8_t)roundf(255 * pixel.r * pixel.a),
                (uint8_t)roundf(255 * pixel.g * pixel.a),
                (uint8_t)roundf(255 * pixel.b * pixel.a));
        }
    }

    SDL_UnlockSurface(pattern_preview);
}

static void update_hit_preview(slot_t* slot)
{
    SDL_LockSurface(pattern_preview);

    for(int x = 0; x < layout.hit_slot.preview_w; x++)
    {
        for(int y = 0; y < layout.hit_slot.preview_h; y++)
        {
            float xf = ((float)x / (layout.hit_slot.preview_w - 1)) * 2 - 1;
            float yf = ((float)y / (layout.hit_slot.preview_h - 1)) * 2 - 1;
            color_t pixel = render_composite_slot_hits(slot, xf, yf);
            ((uint32_t*)(pattern_preview->pixels))[x + layout.hit_slot.preview_w * y] = SDL_MapRGB(
                pattern_preview->format,
                (uint8_t)roundf(255 * pixel.r * pixel.a),
                (uint8_t)roundf(255 * pixel.g * pixel.a),
                (uint8_t)roundf(255 * pixel.b * pixel.a));
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
    if(phase < 1.)
        y = layout.audio.ball_y + layout.audio.ball_r + (layout.audio.ball_h - 2 * layout.audio.ball_r) * dy;
    else
        y = layout.audio.ball_y + layout.audio.ball_r + (layout.audio.ball_h - 2 * layout.audio.ball_r) * (0.4 + 0.6 * dy);

    int x = layout.audio.ball_x + layout.audio.ball_r + (layout.audio.ball_w - 2 * layout.audio.ball_r) * 0.5;
    filledCircleRGBA(audio_pane, x, y, layout.audio.ball_r, 255, 10, 10, 255);
    hlineRGBA(audio_pane, layout.audio.ball_x, layout.audio.ball_x + layout.audio.ball_w, layout.audio.ball_y + layout.audio.ball_h, 255, 255, 255, 255);


    char buf[16];
    snprintf(buf, 16, "bpm: %.2f", timebase_get_bpm());
    SDL_Color white = {255, 255, 255, 255};
    text_render(audio_pane, signal_font, &layout.audio.bpm_txt, &white, buf);

    snprintf(buf, 16, "fps: %d", stat_fps);
    text_render(audio_pane, signal_font, &layout.audio.fps_txt, &white, buf);

    snprintf(buf, 16, "ops: %d", stat_ops);
    text_render(audio_pane, signal_font, &layout.audio.ops_txt, &white, buf);

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

    if(output_strip->bus >= 0)
        color = output_strip->color;

    snprintf(buf, 16, "%d @0x%08X", output_strip->length, output_strip->id);
    text_render(output_pane, filter_font, &layout.output.name_txt, &color, buf);
}

static void ui_update_midi(struct midi_controller * controller){
    rect_t r;
    rect_array_origin(&layout.midi.rect_array, &r);
    SDL_FillRect(midi_pane, &r, SDL_MapRGB(midi_pane->format, 20, 20, 20));

    SDL_Color color = {150, 150, 150, 255};

    if(controller->available)
        color = controller->color;

    text_render(midi_pane, signal_font, &layout.midi.name_txt, &color, controller->short_name);
}

static void ui_update_slot(slot_t* slot)
{
    rect_t r;
    SDL_Color param_name_c = {255, 255, 255, 255};
    rect_array_origin(&layout.slot.rect_array, &r);
    SDL_FillRect(slot_pane, &r, SDL_MapRGB(slot_pane->format, 20, 20, 20));

    if(slot->pattern)
    {
        update_pattern_preview(slot);
        SDL_BlitSurface(pattern_preview, 0, slot_pane, &layout.slot.preview_rect);

        slider_render_alpha(&slot->alpha);
        SDL_BlitSurface(alpha_slider_surface, 0, slot_pane, &layout.slot.alpha_rect);

        for(int i = 0; i < slot->pattern->n_params; i++)
        {
            if(&slot->param_states[i] == active_param_source){
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
            slider_render(&slot->pattern->parameters[i], &slot->param_states[i], param_name_c);
            rect_array_layout(&layout.slot.sliders_rect_array, i, &r);

            SDL_BlitSurface(slider_surface, 0, slot_pane, &r);
        }
        
    }
}

static void ui_update_hit_slot(slot_t* hit_slot)
{
    rect_t r;
    SDL_Color param_name_c = {255, 255, 255, 255};
    rect_array_origin(&layout.hit_slot.rect_array, &r);
    SDL_FillRect(hit_slot_pane, &r, SDL_MapRGB(slot_pane->format, 20, 20, 20));

    if(hit_slot->hit)
    {
        update_hit_preview(hit_slot);
        SDL_BlitSurface(pattern_preview, 0, hit_slot_pane, &layout.hit_slot.preview_rect);

        slider_render_alpha(&hit_slot->alpha);
        SDL_BlitSurface(alpha_slider_surface, 0, hit_slot_pane, &layout.hit_slot.alpha_rect);

        for(int i = 0; i < hit_slot->hit->n_params; i++)
        {
            if(&hit_slot->param_states[i] == active_param_source){
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
            slider_render(&hit_slot->hit->parameters[i], &hit_slot->param_states[i], param_name_c);
            rect_array_layout(&layout.hit_slot.sliders_rect_array, i, &r);

            SDL_BlitSurface(slider_surface, 0, hit_slot_pane, &r);
        }
        
    }
}

static void ui_update_filter(filter_t * filter)
{
    rect_t r;
    SDL_Color white = {255, 255, 255, 255};

    rect_array_origin(&layout.filter.rect_array, &r);
    SDL_FillRect(filter_pane, &r, SDL_MapRGB(filter_pane->format, 20, 20, 20));

    text_render(filter_pane, filter_font, &layout.filter.name_txt, &filter->color, filter->name);

    graph_update(&(filter->graph_state), filter->output.value);
    graph_render(&(filter->graph_state), white);
    SDL_BlitSurface(graph_surface, 0, filter_pane, &layout.graph_filter.rect);
}

static void ui_update_pattern(pattern_t* pattern)
{
    rect_t r;
    rect_array_origin(&layout.add_pattern.rect_array, &r);
    SDL_FillRect(pattern_pane, &r, SDL_MapRGB(pattern_pane->format, 20, 20, 20));

    SDL_Color white = {255, 255, 255, 255};
    text_render(pattern_pane, pattern_font, &layout.add_pattern.name_txt, &white, pattern->name);
}

static void ui_update_hit(hit_t * hit)
{
    rect_t r;
    rect_array_origin(&layout.add_hit.rect_array, &r);
    SDL_FillRect(hit_pane, &r, SDL_MapRGB(hit_pane->format, 20, 20, 20));

    SDL_Color white = {255, 255, 255, 255};
    text_render(hit_pane, pattern_font, &layout.add_hit.name_txt, &white, hit->name);
}

static void ui_update_signal(signal_t* signal)
{
    rect_t r;
    SDL_Color param_name_c;
    SDL_Color signal_c;
    rect_array_origin(&layout.signal.rect_array, &r);
    SDL_FillRect(signal_pane, &r, SDL_MapRGB(signal_pane->format, 20, 20, 20));

    signal_c = color_to_SDL(signal->color);
    text_render(signal_pane, signal_font, &layout.signal.name_txt, &signal_c, signal->name);

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

    for(int i=0; i<n_hit_slots; i++)
    {
        if(hit_slots[i].hit)
        {
            ui_update_hit_slot(&hit_slots[i]);
            rect_array_layout(&layout.hit_slot.rect_array, i, &r);

            if(active_slot.slot == &hit_slots[i]) { //TODO ? shoudl this be active_hit_slot
                rect_shift(&r, &active_slot.dxy);
            }

            SDL_BlitSurface(hit_slot_pane, 0, screen, &r);
        }
    }

    for(int i=0; i<n_patterns; i++)
    {
        ui_update_pattern(patterns[i]);
        rect_array_layout(&layout.add_pattern.rect_array, i, &r);

        if(active_pattern.index == i) {
            rect_shift(&r, &active_pattern.dxy);
        }

        SDL_BlitSurface(pattern_pane, 0, screen, &r);
    }

    for(int i=0; i<n_hits; i++)
    {
        ui_update_hit(hits[i]);
        rect_array_layout(&layout.add_hit.rect_array, i, &r);

        if(active_hit.index == i) {
            rect_shift(&r, &active_hit.dxy);
        }

        SDL_BlitSurface(hit_pane, 0, screen, &r);
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

    for(int i = 0; i < n_controllers_enabled; i++){
        ui_update_midi(&controllers_enabled[i]);
        rect_array_layout(&layout.midi.rect_array, i, &r);
        
        SDL_BlitSurface(midi_pane, 0, screen, &r);
    }

    SDL_Flip(screen);
}

static void mouse_drag_pattern(struct xy xy)
{
    active_pattern.dxy = xy; 
}

static void mouse_drag_hit(struct xy xy)
{
    active_hit.dxy = xy; 
}

static void mouse_drag_slot(struct xy xy)
{
    active_slot.dxy = xy; 
}


static void mouse_drop_pattern()
{
    rect_t r;
    struct xy xy;
    xy = xy_add(active_pattern.dxy, mouse_drag_start);

    for(int i = 0; i < n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(in_rect(&xy, &r, 0)){
            pat_unload(&slots[i]);
            pat_load(&slots[i],patterns[active_pattern.index]);
        }
    }
    active_pattern.index = -1;
}

static void mouse_drop_hit()
{
    rect_t r;
    struct xy xy;
    xy = xy_add(active_hit.dxy, mouse_drag_start);

    for(int i = 0; i < n_hit_slots; i++)
    {
        rect_array_layout(&layout.hit_slot.rect_array, i, &r);
        if(in_rect(&xy, &r, 0)){
            hit_unload(&hit_slots[i]);
            hit_load(&hit_slots[i], hits[active_hit.index]);
        }
    }
    active_hit.index = -1;
}


static void mouse_drop_slot()
{
    rect_t r;
    struct xy xy;
    xy = xy_add(active_slot.dxy, mouse_drag_start);
    int max_x = 0; // XXX This breaks in different layouts

    if(active_slot.is_pattern){
        for(int i = 0; i < n_slots; i++)
        {
            rect_array_layout(&layout.slot.rect_array, i, &r);
            if(in_rect(&xy, &r, 0)){
                slot_t temp_slot = *active_slot.slot;
                *active_slot.slot = slots[i];
                slots[i] = temp_slot;
            }
            max_x = max_x > (r.x + r.w) ? max_x : (r.x + r.w);
        }
        if(xy.x > max_x){
            pat_unload(active_slot.slot);
        }
    }else{
        for(int i = 0; i < n_hit_slots; i++)
        {
            rect_array_layout(&layout.hit_slot.rect_array, i, &r);
            if(in_rect(&xy, &r, 0)){
                slot_t temp_slot = *active_slot.slot;
                *active_slot.slot = hit_slots[i];
                hit_slots[i] = temp_slot;
            }
            max_x = max_x > (r.x + r.w) ? max_x : (r.x + r.w);
        }
        if(xy.x > max_x){
            hit_unload(active_slot.slot);
        }
    }
    active_slot.slot = 0;
}

static int mouse_click_slot(int index, struct xy xy)
{
    rect_t r;
    struct xy offset;
    if(!slots[index].pattern) return 0;

    // See if the click is on the alpha slider
    if(in_rect(&xy, &layout.slot.alpha_rect, &offset)){
        return !!mouse_click_alpha_slider(&slots[index].alpha, offset);
    }

    // See if the click is on the preview 
    if(in_rect(&xy, &layout.slot.preview_rect, &offset)){
        slots[index].pattern->prevclick(
                &slots[index],
                -1.0 + 2.0 * (xy.x - layout.slot.preview_x) / (float) layout.slot.preview_w,
                -1.0 + 2.0 * (xy.y - layout.slot.preview_y) / (float) layout.slot.preview_h);
        return 1; 
    }

    // See if the click is on a parameter slider
    for(int i = 0; i < slots[index].pattern->n_params; i++)
    {
        rect_array_layout(&layout.slot.sliders_rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&slots[index].param_states[i], offset);
        }
    }

    // Else, drag the slot
    active_slot.slot = &slots[index];
    active_slot.is_pattern = 1;
    active_slot.dxy = (struct xy) {0, 0};

    mouse_drag_fn_p = &mouse_drag_slot;
    mouse_drop_fn_p = &mouse_drop_slot;
    return 1;

}

static void hit_release(){
    if(active_active_hit && active_active_hit->hit){
        active_active_hit->hit->event(active_active_hit, HITEV_NOTE_OFF, 1.);
    }
    active_active_hit = 0;
}

static int mouse_click_hit_slot(int index, struct xy xy)
{
    rect_t r;
    struct xy offset;

    if(!hit_slots[index].hit) return 0;

    // See if the click is on the alpha slider
    if(in_rect(&xy, &layout.hit_slot.alpha_rect, &offset)){
        return !!mouse_click_alpha_slider(&hit_slots[index].alpha, offset);
    }

    // See if the click is on the preview 
    if(in_rect(&xy, &layout.hit_slot.preview_rect, &offset)){
        active_active_hit = hit_slots[index].hit->start(&hit_slots[index]);
        if(active_active_hit) active_active_hit->hit->event(active_active_hit, HITEV_NOTE_ON, 1.);
        mouse_drop_fn_p = &hit_release;
        hit_slots[index].hit->prevclick(
                &hit_slots[index],
                -1.0 + 2.0 * (xy.x - layout.hit_slot.preview_x) / (float) layout.hit_slot.preview_w,
                -1.0 + 2.0 * (xy.y - layout.hit_slot.preview_y) / (float) layout.hit_slot.preview_h);
        return 1; 
    }

    // See if the click is on a parameter slider
    for(int i = 0; i < hit_slots[index].hit->n_params; i++)
    {
        rect_array_layout(&layout.hit_slot.sliders_rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&hit_slots[index].param_states[i], offset);
        }
    }

    // Else, drag the slot
    active_slot.slot = &hit_slots[index];
    active_slot.is_pattern = 0;
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
        if(in_rect(&xy, &r, &offset)){
            return !!mouse_click_param_slider(&signals[index].param_states[i], offset);
        }
    }

    // Was the output clicked
    if(in_rect(&xy, &layout.graph_signal.rect, 0)){
        // Is there something looking for a source?
        if(active_param_source){
            param_state_connect(active_param_source, &signals[index].output);
            active_param_source = 0;
        }
    }

    return 0;
}

static int mouse_click_filter(int index, struct xy xy){
    if(in_rect(&xy, &layout.graph_filter.rect, 0)){
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
        if(in_rect(&xy, &r, &offset)){
            if(active_param_source){
                param_state_connect(active_param_source, &waveform_bins[i].output);
                active_param_source = 0;
            }
            return 1;
        }
    }

    if(in_rect(&xy, &layout.waveform.rect, &offset)){
        timebase_tap(0.3);
    }

    if(in_rect(&xy, &layout.audio.ball_rect, &offset)){
        printf("ball clicked\n");
        timebase_align();
    }

    if(in_rect(&xy, &layout.audio.auto_rect, &offset)){
        if(timebase_source == TB_AUTOMATIC)
            timebase_source = TB_MANUAL;
        else 
            timebase_source = TB_AUTOMATIC;
    }

    return 1;
}

static int mouse_click(struct xy xy)
{
    struct xy offset;
    rect_t r;
    
    // See if click is in master pane
    if(in_rect(&xy, &layout.master.rect, &offset)){
        return 1;
    }

    // See if click is in audio pane
    if(in_rect(&xy, &layout.audio.rect, &offset)){
        return mouse_click_audio(offset);
    }


    // See if click is in a slot
    for(int i=0; i<n_slots; i++)
    {
        rect_array_layout(&layout.slot.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            // If it is, see if that slot wants to handle it
            return mouse_click_slot(i, offset);
        }
    }

    // See if click is in a hit slot
    for(int i=0; i<n_hit_slots; i++)
    {
        rect_array_layout(&layout.hit_slot.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            // If it is, see if that slot wants to handle it
            return mouse_click_hit_slot(i, offset);
        }
    }

    // See if click is in a pattern
    for(int i=0; i<n_slots; i++)
    {
        rect_array_layout(&layout.add_pattern.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            active_pattern.index = i;
            active_pattern.dxy = (struct xy) {0,0};
            mouse_drag_fn_p = &mouse_drag_pattern;
            mouse_drop_fn_p = &mouse_drop_pattern;
            return 1;
        }
    }

    // See if click is in a hit
    for(int i=0; i<n_hit_slots; i++)
    {
        rect_array_layout(&layout.add_hit.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            active_hit.index = i;
            active_hit.dxy = (struct xy) {0,0};
            mouse_drag_fn_p = &mouse_drag_hit;
            mouse_drop_fn_p = &mouse_drop_hit;
            return 1;
        }
    }

    // See if click is in an signal
    for(int i = 0; i < n_signals; i++){
        rect_array_layout(&layout.signal.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return mouse_click_signal(i, offset);
        }
    }

    // See if click is in filter
    for(int i = 0; i < n_filters; i++){
        rect_array_layout(&layout.filter.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return mouse_click_filter(i, offset);
        }
    }

    // See if click is in output
    for(int i =  0; i < n_output_strips; i++){
        rect_array_layout(&layout.output.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return mouse_click_output(i, offset);
        }
    }

    // See if click is in midi
    for(int i =  0; i < n_controllers_enabled; i++){
        rect_array_layout(&layout.midi.rect_array, i, &r);
        if(in_rect(&xy, &r, &offset)){
            return mouse_click_midi(i, offset);
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
    struct midi_event * me;
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
                mouse_click(xy);
                mouse_down = 1;
                mouse_drag_start = xy;
                break;
            case SDL_MOUSEMOTION:
                if(mouse_down && mouse_drag_fn_p)
                {
                    xy  = xy_sub(xy, mouse_drag_start);
                    (*mouse_drag_fn_p)(xy);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(mouse_drop_fn_p) (*mouse_drop_fn_p)();
                mouse_drag_fn_p = 0;
                mouse_drop_fn_p = 0;
                mouse_down = 0;
                break;
            case SDL_MIDI_NOTE_ON:
                me = e.user.data1;
                printf("Note on: %d %d %d %d\n", me->device, me->event, me->data1, me->data2);
                midi_handle_note_event(me);
                free(me);
                break;
            case SDL_MIDI_NOTE_OFF:
                me = e.user.data1;
                printf("Note off: %d %d %d %d\n", me->device, me->event, me->data1, me->data2);
                midi_handle_note_event(me);
                free(me);
                break;
        }
    }
    return 1;
}

