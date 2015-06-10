#include <math.h>

#include <SDL/SDL.h>

#include "util/color.h"
#include "util/math.h"

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
    param = MIN(MAX(param, 0.), 1.);
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
#define MINP 0.0
    color_t result;
    param = MIN(MAX(param, 0.), 1.);
    HSVtoRGB(&result.r, &result.g, &result.b, param * 360, 1.0, 1.0);
    float p = result.r + result.g + result.b;
    p /= (1. - MINP);
    result.r /= p;
    result.g /= p;
    result.b /= p;
    result.r += MINP;
    result.g += MINP;
    result.b += MINP;
    result.a = 1;
    return result;
}

// --- Colormaps ---

int n_colormaps = 3;
struct colormap * colormaps[3] =  {&cm_rainbow, &cm_rainbow_edged, &cm_rainbow_equal};

color_t colormap_color(struct colormap * cm, float value){
    // TODO: move test into init
    if(colormap_test(cm)) return (color_t) {0,0,0,1};
    value = MIN(MAX(value, 0.), 1.);

    struct colormap_el * left = cm->points;
    struct colormap_el * right = left+1;

    while(right->x < value){
        left++;
        right++;
    }

    float t = powf((value - left->x) / (right->x - left->x), left->gamma);

    color_t out;
    out.r = right->y.r * t + left->y.r * (1.0 - t);
    out.g = right->y.g * t + left->y.g * (1.0 - t);
    out.b = right->y.b * t + left->y.b * (1.0 - t);
    out.a = right->y.a * t + left->y.a * (1.0 - t);
    out.a = 1;

    return out;
}

int colormap_test(struct colormap * cm){
    // Test to make sure the colormap is valid:
    // 1. There are at least 2 points
    // 2. The first point has x = 0
    // 3. The last point has x = 1
    // 4. The x's are monotonically increasing
    // 5. All of the color channels are on [0, 1]
    // 6. Each gamma is positive
    if(!cm) return -1;
    if(cm->n_points < 2) return -1;
    if(cm->points[0].x != 0) return -1;
    if(cm->points[cm->n_points-1].x != 1) return -1;
    float last_x = 0;
    for(size_t i = 0; i < cm->n_points; i++){
        if(cm->points[i].x < last_x) return -1;
        if((cm->points[i].y.r < 0) || (cm->points[i].y.r > 1)) return -1;
        if((cm->points[i].y.g < 0) || (cm->points[i].y.g > 1)) return -1;
        if((cm->points[i].y.b < 0) || (cm->points[i].y.b > 1)) return -1;
        if(cm->points[i].gamma <= 0) return -1;
        last_x = cm->points[i].x;
    }
    return 0;
}

struct colormap cm_rainbow = {
    .name = "Rainbow",
    .n_points = 6,
    .points = {
        { .x = 0.,
          .y = {1., 0., 0., 1.},
          .gamma = 1.,
        },
        { .x = .2,
          .y = {1., 1., 0., 1.},
          .gamma = 1.,
        },
        { .x = .4,
          .y = {0., 1., 0., 1.},
          .gamma = 1.,
        },
        { .x = .6,
          .y = {0., 1., 1., 1.},
          .gamma = 1.,
        },
        { .x = .8,
          .y = {0., 0., 1., 1.},
          .gamma = 1.,
        },
        { .x = 1.,
          .y = {1., 0., 1., 1.},
          .gamma = 1.,
        },
    },
};

struct colormap cm_rainbow_edged = {
    .name = "Rainbow Edged",
    .n_points = 8,
    .points = {
        { .x = 0.0,
          .y = {0., 0., 0., 1.},
          .gamma = 1.,
        },
        { .x = 0.05,
          .y = {1., 0., 0., 1.},
          .gamma = 1.,
        },
        { .x = .2,
          .y = {1., 1., 0., 1.},
          .gamma = 1.,
        },
        { .x = .4,
          .y = {0., 1., 0., 1.},
          .gamma = 1.,
        },
        { .x = .6,
          .y = {0., 1., 1., 1.},
          .gamma = 1.,
        },
        { .x = .8,
          .y = {0., 0., 1., 1.},
          .gamma = 1.,
        },
        { .x = 0.95,
          .y = {1., 0., 1., 1.},
          .gamma = 1.,
        },
        { .x = 1.0,
          .y = {1., 1., 1., 1.},
          .gamma = 1.,
        },
    },
};

struct colormap cm_rainbow_equal = {
    .name = "Rainbow Equal",
    .n_points = 6,
    .points = {
        { .x = 0.,
          .y = {1., 0., 0., 1.},
          .gamma = 1.,
        },
        { .x = .2,
          .y = {0.5, 0.5, 0., 1.},
          .gamma = 1.,
        },
        { .x = .4,
          .y = {0., 1., 0., 1.},
          .gamma = 1.,
        },
        { .x = .6,
          .y = {0., 0.5, 0.5, 1.},
          .gamma = 1.,
        },
        { .x = .8,
          .y = {0., 0., 1., 1.},
          .gamma = 1.,
        },
        { .x = 1.,
          .y = {0.5, 0., 0.5, 1.},
          .gamma = 1.,
        },
    },
};
