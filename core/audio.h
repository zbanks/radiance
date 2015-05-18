#ifndef __AUDIO_H
#define __AUDIO_H

#define NUM_CHANNELS 1
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 128

typedef float* chunk_pt;
void audio_start();
void audio_stop();

#endif
