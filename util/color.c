#include <math.h>

#include <SDL/SDL.h>

#include "util/color.h"

uint32_t color_to_MapRGB(const SDL_PixelFormat * format, color_t color){
    return SDL_MapRGB(format, 
                      (uint8_t) roundf(255 * color.r),
                      (uint8_t) roundf(255 * color.g),
                      (uint8_t) roundf(255 * color.b));
}

SDL_Color color_to_SDL(color_t color){
    return (SDL_Color) {(uint8_t) roundf(255 * color.r),
                        (uint8_t) roundf(255 * color.g),
                        (uint8_t) roundf(255 * color.b),
                        (uint8_t) roundf(255 * color.a)};
}


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

color_t param_to_color(float param)
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
        HSVtoRGB(&result.r, &result.g, &result.b, param * 300, 1.0, 1.0);
    }
    result.a = 1;
    return result;
}

color_t param_to_cpow_color(float param){
    color_t result;
    HSVtoRGB(&result.r, &result.g, &result.b, param * 360, 1.0, 1.0);
    float p = result.r + result.g + result.b;
    result.r /= p;
    result.g /= p;
    result.b /= p;
    result.a = 1;
    return result;
}
