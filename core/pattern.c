#include "pattern.h"
#include "slot.h"
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define N_PATTERNS 3

const int n_patterns = N_PATTERNS;
const pattern_t* patterns[N_PATTERNS] = {&pat_full, &pat_wave, &pat_bubble};

static void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v );

// Do we actually want this? -@zbanks, 4/4/15
static int get_param_by_name(slot_t * slot, const char * name, float * param){
    for(int i = 0; i < slot->pattern->n_params; i++){
        if(strcmp(slot->pattern->parameters[i].name,  name) == 0){
            *param = slot->param_values[i];
            return 0;
        }
    }
    return 1;
}

static color_t param_to_color(float param)
{
#define ENDSZ 0.07
    color_t result;
    if(param < ENDSZ){
        result.r = param / ENDSZ;
        result.g = 0;
        result.b = 0;
    }else if(param > (1.0 - ENDSZ)){
        result.r = 1.0;
        result.g = (param - 1.0 + ENDSZ) / ENDSZ;
        result.b = 1.0;
    }else{
        param = (param - ENDSZ) / (1.0 - 2 * ENDSZ);
        HSVtoRGB(&result.r, &result.g, &result.b, param * 360, 1.0, 1.0);
    }
    result.a = 1;
    return result;
}

// --------- Pattern: Full -----------

pat_state_pt pat_full_init()
{
    return malloc(sizeof(color_t));
}

void pat_full_del(pat_state_pt state)
{
    free(state);
}

void pat_full_update(slot_t* slot, float t)
{
    color_t* color = (color_t*)slot->state;
    *color = param_to_color(slot->param_values[0]);
}

void pat_full_prevclick(slot_t * slot, float x, float y){

}

color_t pat_full_pixel(slot_t* slot, float x, float y)
{
    return *(color_t*)slot->state;
}

const parameter_t pat_full_params[] = {
    {
        .name = "Color",
        .default_val = 0.5,
    },
};

const pattern_t pat_full = {
    .render = &pat_full_pixel,
    .init = &pat_full_init,
    .del = &pat_full_del,
    .update = &pat_full_update,
    .prevclick = &pat_full_prevclick,
    .n_params = sizeof(pat_full_params) / sizeof(parameter_t),
    .parameters = pat_full_params,
    .name = "Full",
};

// --------- Pattern: Wave -----------

typedef struct
{
    color_t color;
    float phase;
    float last_t;
    float kx;
    float ky;
} pat_wave_state_t;

pat_state_pt pat_wave_init()
{
    return malloc(sizeof(pat_wave_state_t));
}

void pat_wave_del(pat_state_pt state)
{
    free(state);
}

void pat_wave_update(slot_t* slot, float t)
{
    float k_mag;
    float k_ang;

    pat_wave_state_t* state = (pat_wave_state_t*)slot->state;
    state->color = param_to_color(slot->param_values[0]);

    state->phase += (t - state->last_t) * slot->param_values[1];
    state->last_t = t;

    k_mag = slot->param_values[2] * 3 + 0.5;
    k_ang = slot->param_values[3] * 2 * M_PI;
    state->kx = cos(k_ang) * k_mag;
    state->ky = sin(k_ang) * k_mag;
}

void pat_wave_prevclick(slot_t * slot, float x, float y){
    slot->param_values[2] = sqrt(pow(x, 2) + pow(y, 2)) / sqrt(2.0);
    slot->param_values[3] = (atan2(y, x) / (2.0 * M_PI)) + 0.5;
}

color_t pat_wave_pixel(slot_t* slot, float x, float y)
{
    pat_wave_state_t* state = (pat_wave_state_t*)slot->state;
    color_t result = state->color;
    result.a = (sin((state->phase + y * state->ky + x * state->kx) * 1 * M_PI) + 1) / 2;
    return result;
}

const parameter_t pat_wave_params[] = {
    {
        .name = "Color",
        .default_val = 0.5,
    },
    {
        .name = "\\omega",
        .default_val = 0.5,
    },
    {
        .name = "|k|",
        .default_val = 0.5,
    },
    {
        .name = "<)k",
        .default_val = 0.5,
    },
};

const pattern_t pat_wave = {
    .render = &pat_wave_pixel,
    .init = &pat_wave_init,
    .del = &pat_wave_del,
    .update = &pat_wave_update,
    .prevclick = &pat_wave_prevclick,
    .n_params = sizeof(pat_wave_params) / sizeof(parameter_t),
    .parameters = pat_wave_params,
    .name = "Wave",
};

// --------- Pattern: Bubble -----------

typedef struct
{
    color_t color;
    float r;
    float rho;
    float cx;
    float cy;
} pat_bubble_state_t;

pat_state_pt pat_bubble_init()
{
    return malloc(sizeof(pat_bubble_state_t));
}

void pat_bubble_del(pat_state_pt state)
{
    free(state);
}

void pat_bubble_update(slot_t* slot, float t)
{
    pat_bubble_state_t* state = (pat_bubble_state_t*)slot->state;
    state->color = param_to_color(slot->param_values[0]);
    state->r = slot->param_values[1];
    state->rho = slot->param_values[2] * 1.3 + 0.3;
    state->cx = slot->param_values[3] * 2 - 1.0;
    state->cy = slot->param_values[4] * 2 - 1.0;
}

void pat_bubble_prevclick(slot_t * slot, float x, float y){
    slot->param_values[3] = (x + 1.0) / 2;
    slot->param_values[4] = (y + 1.0) / 2;
}

color_t pat_bubble_pixel(slot_t* slot, float x, float y)
{
    float d;
    pat_bubble_state_t* state = (pat_bubble_state_t*)slot->state;
    color_t result = state->color;

    d = sqrt(pow(state->cx - x, 2) + pow(state->cy - y, 2)) / state->r;
    
    if(d < 1.0)
        result.a = pow(1.0 - pow(d, state->rho), 1.0 / state->rho);
    else
        result.a = 0.0;
    return result;
}

const parameter_t pat_bubble_params[] = {
    {
        .name = "Color",
        .default_val = 0.5,
    },
    {
        .name = "r",
        .default_val = 0.5,
    },
    {
        .name = "\\rho",
        .default_val = 0.5,
    },
    {
        .name = "cx",
        .default_val = 0.5,
    },
    {
        .name = "cy",
        .default_val = 0.5,
    },
};

const pattern_t pat_bubble = {
    .render = &pat_bubble_pixel,
    .init = &pat_bubble_init,
    .del = &pat_bubble_del,
    .update = &pat_bubble_update,
    .prevclick = &pat_bubble_prevclick,
    .n_params = sizeof(pat_bubble_params) / sizeof(parameter_t),
    .parameters = pat_bubble_params,
    .name = "Bubble",
};


/*
pat_state_pt init()
{
    return malloc(sizeof(float));
}

void del(pat_state_pt state)
{
    free((float*)state);
}

void update(slot_t* slot, float t)
{
    float* st = (float*)slot->state;
    *st = t;
}

color_t pixel_at(slot_t* slot, float x, float y)
{
    float* st = (float*)slot->state;
    color_t result;
    float n = 1-sqrt(x*x+y*y);
    if(n < 0) n = 0;

    result.r = 1;
    result.g = slot->param_values[0];
    result.b = 0;
    result.a = n;

    return result;
}

color_t pixel_at2(slot_t* slot, float x, float y)
{
    float* st = (float*)slot->state;

    color_t result;
    float n = 1 - x*x;
    if(n < 0) n = 0;

    result.r = 0;
    result.g = 0;
    result.b = 1;
    result.a = n * fmod(*st, 1);

    return result;
}

const parameter_t ball_parameters[1] = {
    {
        .name = "yellow",
        .default_val = 0.5,
    },
};

const pattern_t ball = {
    .render = &pixel_at,
    .init = &init,
    .del = &del,
    .update = &update,
    .n_params = 1,
    .parameters = ball_parameters,
};

const pattern_t stripe = {
    .render = &pixel_at2,
    .init = &init,
    .del = &del,
    .update = &update,
    .n_params = 0,
};
*/

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)
static void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;
	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}
	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
}

