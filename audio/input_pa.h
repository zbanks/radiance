#ifndef __AUDIO_INPUT_PA_H__
#define __AUDIO_INPUT_PA_H__

#include "audio/audio.h"

int audio_pa_run(audio_callback_fn_pt callback, double sample_rate,  unsigned long chunk_size);

#endif
