#ifndef __PATTERNS_STATIC_H__
#define __PATTERNS_STATIC_H__

void pattern_init();
void pattern_del();

extern int n_patterns;
extern pattern_t** patterns;

extern pattern_t pat_full;
extern pattern_t pat_wave;
extern pattern_t pat_bubble;
extern pattern_t pat_strobe;
extern pattern_t pat_fade;
extern pattern_t pat_swipe;
extern pattern_t pat_rainbow;
extern pattern_t pat_sparkle;
extern pattern_t pat_perlin;
extern pattern_t pat_psparkle;

#endif
