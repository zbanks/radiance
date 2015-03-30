#include "audio.h"
#include <stdlib.h>
#include <stdio.h>
#include <portaudio.h>

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

#include "err.h"
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

static int audio_running;

static SDL_Thread* audio_thread;

#define NUM_CHANNELS 1
#define PA_SAMPLE_TYPE paFloat32
#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 512

static float chunk[NUM_CHANNELS * FRAMES_PER_BUFFER];
static float fifo[NUM_CHANNELS * FRAMES_PER_BUFFER * 2];

using namespace std;

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

#define VAMP_PLUGIN_SO ("btrack.so")
#define VAMP_PLUGIN_ID ("btrack-vamp")

static int audio_run(void* args)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream = NULL;
    PaError err;

    PluginLoader::PluginKey key;
    int elapsed = 0;
    int returnValue = 1;
    RealTime rt;
    PluginWrapper *wrapper = 0;
    RealTime adjustment = RealTime::zeroTime;
    PluginLoader *loader = PluginLoader::getInstance();
    Plugin *plugin;
    Plugin::OutputDescriptor od;

    key = loader->composePluginKey(VAMP_PLUGIN_SO, VAMP_PLUGIN_ID);
    plugin = loader->loadPlugin(key, SAMPLE_RATE, PluginLoader::ADAPT_ALL_SAFE);
    if(!plugin){
        printf("Unable to load vamp plugin\n");
        return 1;
    }
    printf("Loaded vamp plugin: %s\n", plugin->getIdentifier().c_str());

    int blockSize = plugin->getPreferredBlockSize();
    int stepSize = plugin->getPreferredStepSize();
    printf("Preffered block/step sizes: %d/%d\n", blockSize, stepSize);

    Plugin::OutputList outputs = plugin->getOutputDescriptors();
    od = outputs[0];
    printf("Output is '%s'\n", od.identifier.c_str());

    if(!plugin->initialise(1, stepSize, blockSize)){
        printf("Error initing vamp plugin\n");
        return 1;
    }
    if(plugin->getInputDomain() != Plugin::TimeDomain){
        printf("Input not in time domain\n");
        return 1;
    }

    
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

    float *fifoptr[1] = {fifo};

    while(audio_running)
    {
        err = Pa_ReadStream( stream, chunk, FRAMES_PER_BUFFER );

        // shift buffer along a step size
    	for (int i=stepSize; i<blockSize; i++) fifo[i-stepSize] = fifo[i];
    	// add new step onto end
    	for (int i=0; i<stepSize; i++) fifo[blockSize-stepSize+i] = chunk[i];
        
        if(err != paNoError) FAIL("Could not read audio chunk\n");
        // Do chunk things here
        rt = RealTime::frame2RealTime(elapsed*FRAMES_PER_BUFFER, SAMPLE_RATE);
        Plugin::FeatureSet features = plugin->process(fifoptr, rt);
        if(!features[0].empty()){
            printf("feat %s %d.%d\n", features[0][0].label.c_str(), features[0][0].timestamp.sec, features[0][0].timestamp.usec());
        }

        elapsed++;
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

