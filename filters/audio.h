#ifndef __AUDIO_H
#define __AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef float * chunk_t;
    extern double last_odf;
    void audio_start();
    void audio_stop();
    void audio_history(float * history, int n_history);

#ifdef __cplusplus
}
#endif

#endif
