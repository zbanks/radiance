#include "midi.h"

#include "err.h"
#include "portmidi.h"
#include "porttime.h"
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

#define MIDI_BUFFER_SIZE 64

static int midi_running;
static SDL_Thread* midi_thread;

static int midi_run(void* args)
{
    PmError err;
    //PtError terr;

    err = Pm_Initialize();
    if(err != pmNoError) FAIL("Could not initialize PortMIDI: %s\n", Pm_GetErrorText(err));

    //terr = Pt_Start(1, 0, 0);
    //if(terr != ptNoError) FAIL("Could not start PortTime: %d\n", terr);

    PortMidiStream* nk;
    PmEvent nk_events[MIDI_BUFFER_SIZE];

    int n = Pm_CountDevices();
    printf("%d midi devices found\n",n);
    for(int i = 0; i < n; i++)
    {
        PmDeviceInfo* device = Pm_GetDeviceInfo(i);
        if(device->input)
        {
            printf("%d: %s\n", i, device->name);
        }
        if(device->input && strcmp(device->name,"nanoPAD2 MIDI 1") == 0)
        {
            err = Pm_OpenInput(&nk,
                               i,
                               0,
                               MIDI_BUFFER_SIZE,
                               0,
                               0);
            if(err != pmNoError) FAIL("Could not open MIDI device: %s\n", Pm_GetErrorText(err));


            printf("found nanokontrol\n");
        }
    }

    while(midi_running)
    {
        int n = Pm_Read(nk, nk_events, MIDI_BUFFER_SIZE);
        for(int i = 0; i < n; i++)
        {
            printf("Event %d\n", nk_events[i].message);
        }
        SDL_Delay(1);
    }

    err = Pm_Close(nk);
    if(err != pmNoError) FAIL("Could not close MIDI device: %s\n", Pm_GetErrorText(err));

    err = Pm_Terminate();
    if(err != pmNoError) FAIL("Could not terminate PortMIDI: %s\n", Pm_GetErrorText(err));

    //terr = Pt_Stop();
    //if(terr != ptNoError) FAIL("Could not stop PortTime: %d\n", terr);

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

