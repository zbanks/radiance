#include <portmidi.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

#include "core/err.h"
#include "midi/controllers.h"
#include "midi/midi.h"

#define MIDI_BUFFER_SIZE 64

static int midi_running;
static SDL_Thread* midi_thread;

static PortMidiStream** streams;
static PmEvent events[MIDI_BUFFER_SIZE];

static int midi_run(void* args)
{
    PmError err;

    err = Pm_Initialize();
    if(err != pmNoError) FAIL("Could not initialize PortMIDI: %s\n", Pm_GetErrorText(err));

    streams = malloc(sizeof(PortMidiStream*) * n_controllers_enabled);
    if(!streams) FAIL("Could not allocate MIDI controller streams");

    for(int i = 0; i < n_controllers_enabled; i++)
    {
        streams[i] = 0;
    }

    int n = Pm_CountDevices();
    for(int i = 0; i < n; i++)
    {
        PmDeviceInfo* device = Pm_GetDeviceInfo(i);
        if(device->input)
        {
            for(int j = 0; j < n_controllers_enabled; j++)
            {
                if(strcmp(device->name, controllers_enabled[j]) == 0)
                {
                    err = Pm_OpenInput(&streams[j], i, 0, MIDI_BUFFER_SIZE, 0, 0);
                    if(err != pmNoError) FAIL("Could not open MIDI device: %s\n", Pm_GetErrorText(err));
                }
            }
        }
    }

    for(int i = 0; i < n_controllers_enabled; i++)
    {
        if(!streams[i])
        {
            printf("WARNING: Could not find MIDI device \"%s\"\n", controllers_enabled[i]);
        }
    }

    while(midi_running)
    {
        for(int i = 0; i < n_controllers_enabled; i++)
        {
            if(!streams[i]) continue;
            int n = Pm_Read(streams[i], events, MIDI_BUFFER_SIZE);
            for(int j = 0; j < n; j++)
            {
                PmMessage m = events[j].message;
                printf("Device %d event %d %d %d\n", i, Pm_MessageStatus(m), Pm_MessageData1(m), Pm_MessageData2(m));
            }
        }
        SDL_Delay(1); // TODO SDL rate limiting
    }

    for(int i = 0; i < n_controllers_enabled; i++)
    {
        if(!streams[i]) continue;
        err = Pm_Close(streams[i]);
        if(err != pmNoError) FAIL("Could not close MIDI device: %s\n", Pm_GetErrorText(err));
    }

    err = Pm_Terminate();
    if(err != pmNoError) FAIL("Could not terminate PortMIDI: %s\n", Pm_GetErrorText(err));

    return 0;
}

void midi_start()
{
    midi_running = 1;
    midi_thread = SDL_CreateThread(&midi_run, 0);
    if(!midi_thread) FAIL("Could not create MIDI thread: %s\n",SDL_GetError());
}

void midi_stop()
{
    midi_running = 0;

    SDL_WaitThread(midi_thread, 0);
}

