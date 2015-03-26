#include "audio.h"
#include <stdlib.h>
#include <stdio.h>
#include <portaudio.h>

//#include <vamp-hostsdk/PluginHostAdapter.h>
//#include <vamp-hostsdk/PluginInputDomainAdapter.h>
//#include <vamp-hostsdk/PluginLoader.h>

#include "err.h"
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

int audio_running;

static SDL_Thread* audio_thread;

#define NUM_CHANNELS 1
#define PA_SAMPLE_TYPE paInt16
#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 256

static int16_t chunk[NUM_CHANNELS * FRAMES_PER_BUFFER];

static int audio_run(void* args)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream = NULL;
    PaError err;

    err = Pa_Initialize();
    if(err != paNoError) FAIL("Could not initialize PortAudio\n");
    inputParameters.device = Pa_GetDefaultInputDevice();

    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream,
                        &inputParameters,
                        0,
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,
                        0,
                        0);
    if(err != paNoError) FAIL("Could not open PortAudio input stream\n");

    err = Pa_StartStream( stream );
    if(err != paNoError) FAIL("Could not open audio input stream\n");

    while(audio_running)
    {
        err = Pa_ReadStream( stream, chunk, FRAMES_PER_BUFFER );
        if(err != paNoError) FAIL("Could not read audio chunk\n");
        // Do chunk things here
    }

    err = Pa_StopStream(stream);
    if(err != paNoError) FAIL("Could not read audio chunk");

    return 0;
}

void audio_start()
{
    audio_running = 1;

    audio_thread = SDL_CreateThread(&audio_run, 0);
    if(!audio_thread) FAIL("Could not create output thread: %s\n",SDL_GetError());
}

void audio_stop()
{
    audio_running = 0;

    SDL_WaitThread(audio_thread, 0);
}

