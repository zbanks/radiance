#include "filters/audio.h"
#include <stdlib.h>
#include <stdio.h>
#include <portaudio.h>

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

#include "core/err.h"
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

#include <algorithm>
#include <deque>
#include <math.h>

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

double last_odf;
std::deque<double> odf_history;
#define ODF_HISTORY_SIZE 512

#define VAMP_PLUGIN_SO ("btrack.so")
#define VAMP_PLUGIN_ID ("btrack-vamp")

void audio_history(float * history, int n_history){
    for(int i = 0; (i < odf_history.size()) && (i < n_history); i++){
        history[i] = odf_history[i];
    }
}

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

    // Clear out the ODF history
    odf_history.clear();
    odf_history.insert(odf_history.begin(), ODF_HISTORY_SIZE, 0.0);

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

        double v = fabs(features[1][0].values[0]);
        odf_history.pop_back();
        odf_history.push_front(max(v, v * 0.2 + odf_history[0] * 0.8));

        double max_odf = *max_element(odf_history.begin(), odf_history.end());
        last_odf = odf_history[0] / (max_odf + 1e-6);

        if(!features[0].empty()){
            printf("feat %s %d.%d %f\n", features[0][0].label.c_str(), features[0][0].timestamp.sec, features[0][0].timestamp.usec(), last_odf);
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

