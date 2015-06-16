#include "audio/audio.h"
#include "audio/input_pa.h"
#include "core/err.h"
#include <portaudio.h>

#define NUM_CHANNELS 1
#define PA_SAMPLE_TYPE paFloat32
static float * chunk;

int audio_pa_run(audio_callback_fn_pt callback, double sample_rate, unsigned long chunk_size){
    chunk = malloc(chunk_size * sizeof(float));
    if(!chunk) FAIL("Could not malloc audio chunk.\n");

    PaError err = Pa_Initialize();
    if(err != paNoError) FAIL("Could not initialize PortAudio\n");

    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaStream *stream = NULL;
    err = Pa_OpenStream(&stream,
                        &inputParameters,
                        0,
                        sample_rate,
                        chunk_size,
                        paClipOff,
                        0,
                        0);
    if(err != paNoError) FAIL("Could not open PortAudio input stream\n");

    err = Pa_StartStream(stream);
    if(err != paNoError) FAIL("Could not open audio input stream\n");

    printf("Gracefully terminated PortAudio\n");

    int cb_err = 0;
    while(cb_err == 0){
        err = Pa_ReadStream(stream, chunk, chunk_size );
        if(err != paNoError) FAIL("Could not read audio chunk\n");
        cb_err = callback(chunk);
    }

    free(chunk);
    return 0;
}
