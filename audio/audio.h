#ifndef __AUDIO_H
#define __AUDIO_H
#include "util/common.h"
NOT_CXX
typedef const float * chunk_pt;
typedef int (*audio_callback_fn_pt)(const chunk_pt chunk);

void audio_start();
void audio_stop();
CXX_OK
#endif
