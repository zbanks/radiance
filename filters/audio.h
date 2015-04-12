#ifndef __AUDIO_H
#define __AUDIO_H

#define NUM_CHANNELS 1
#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 512

typedef float * chunk_p;
void audio_start();
void audio_stop();

#endif
