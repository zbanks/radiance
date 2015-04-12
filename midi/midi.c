#include <portmidi.h>
#include <string.h>

#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>

#include "core/err.h"
#include "midi/controllers.h"
#include "midi/midi.h"
#include "core/parameter.h"
#include "ui/ui.h"

#define MIDI_BUFFER_SIZE 64
#define MIN(x, y) ((x > y) ? y : x)
#define MAX(x, y) ((x > y) ? x : y)

#define MIDI_SELECT_DATA_THRESHOLD 32  // 1/4  of the range
#define MIDI_SELECT_RATIO_THRESHOLD 0.15

static int midi_running;
static SDL_Thread* midi_thread;

static PortMidiStream** streams;
static PmEvent events[MIDI_BUFFER_SIZE];

static struct midi_connection_table * connection_table = 0;

int n_recent_events = 0;

static struct recent_event {
    unsigned char device;   
    PmEvent event;
} recent_events[MIDI_BUFFER_SIZE];

static struct {
    unsigned char device;
    unsigned char event;
    unsigned char data1;
    unsigned char data2_max;
    unsigned char data2_min;
    long time_max;
    long time_min;
} collapsed_events[MIDI_BUFFER_SIZE];

static void midi_connect_param(unsigned char device, unsigned char event, unsigned char data1){
    printf("Connecting to midi %d %d %d\n", device, event, data1); 
    n_recent_events  = 0;

    struct midi_connection_table * ct = connection_table;
    while(ct){
        if((ct->device == device) && (ct->event == event)){
            param_state_connect(active_param_source, &ct->outputs[(int) data1]);
            active_param_source = 0;
            return;
        }
        ct = ct->next;
    }
    
    // No table element, create one.
    ct = malloc(sizeof(struct midi_connection_table));
    char * strings = malloc(N_DATA1 * 5); // This is never free'd
    ct->device = device;
    ct->event = event;
    ct->next = connection_table;
    connection_table = ct;

    // Init outputs
    for(int i = 0; i < N_DATA1; i++){
        snprintf(strings, 5, "%d", i);
        ct->outputs[i].handle_color.r = 0;
        ct->outputs[i].handle_color.g = 255;
        ct->outputs[i].handle_color.b = MAX(40 + device * 40, 255);
        ct->outputs[i].label_color.r = 0;
        ct->outputs[i].label_color.g = 255;
        ct->outputs[i].label_color.b = MAX(40 + device * 40, 255);
        ct->outputs[i].label = strings;
        strings += 5;
    }

    param_state_connect(active_param_source, &ct->outputs[(int) data1]);
    active_param_source = 0;
}

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

    int n_collapsed_events;

    while(midi_running)
    {
        for(int i = 0; i < n_controllers_enabled; i++)
        {
            if(!streams[i]) continue;
            int n = Pm_Read(streams[i], events, MIDI_BUFFER_SIZE);
            if(n < 0){
                printf("Read error!\n");
                continue;
            }
            for(int j = 0; j < n; j++)
            {
                PmMessage m = events[j].message;
                unsigned char event = Pm_MessageStatus(m);
                unsigned char data1 = Pm_MessageData1(m);
                unsigned char data2 = Pm_MessageData2(m);

                //printf("Device %d event %d %d %d %li\n", i, event, data1, data2, events[j].timestamp);

                struct midi_connection_table * ct = connection_table;
                while(ct){
                    if((ct->device == i) && (ct->event == event)){
                        param_output_set(&ct->outputs[(int) data1], data2 / 127.);
                        break;
                    }
                    ct = ct->next;
                }

                if(active_param_source){
                    memcpy(&recent_events[1], &recent_events[0], sizeof(struct recent_event) * (MIDI_BUFFER_SIZE - 1));
                    memcpy(&recent_events[0].event, &events[j], sizeof(PmEvent));
                    recent_events[0].device = i;

                    n_recent_events = (n_recent_events >= MIDI_BUFFER_SIZE) ? MIDI_BUFFER_SIZE : n_recent_events + 1;

                    n_collapsed_events = 0;
                    memset(collapsed_events, 0, sizeof(collapsed_events));

                    for(int k = 0; k < n_recent_events; k++){
                        event = Pm_MessageStatus(recent_events[k].event.message);
                        data1 = Pm_MessageData1(recent_events[k].event.message);
                        data2 = Pm_MessageData2(recent_events[k].event.message);

                        for(int l = 0; l <  n_collapsed_events; l++){
                            if((event == collapsed_events[l].event) &&
                               (data1 == collapsed_events[l].data1) &&
                               (recent_events[k].device == collapsed_events[l].device)){

                                collapsed_events[l].data2_max = MAX(collapsed_events[l].data2_max, data2);
                                collapsed_events[l].data2_min = MIN(collapsed_events[l].data2_min, data2);
                                collapsed_events[l].time_max = MAX(collapsed_events[l].time_max, recent_events[k].event.timestamp);
                                collapsed_events[l].time_min = MIN(collapsed_events[l].time_min, recent_events[k].event.timestamp);
                                goto has_collapsed_event;
                            }
                        }
                        // Add to collapsed_events
                        collapsed_events[n_collapsed_events].event = event;
                        collapsed_events[n_collapsed_events].data1 = data1;
                        collapsed_events[n_collapsed_events].device = recent_events[k].device;
                        collapsed_events[n_collapsed_events].data2_max = data2;
                        collapsed_events[n_collapsed_events].data2_min = data2;
                        collapsed_events[n_collapsed_events].time_max = recent_events[k].event.timestamp;
                        collapsed_events[n_collapsed_events].time_min = recent_events[k].event.timestamp;
                        n_collapsed_events++;

has_collapsed_event:
                        continue;

                    }

                    for(int l = 0; l < n_collapsed_events; l++){
                        if(((collapsed_events[l].data2_max - collapsed_events[l].data2_min) > MIDI_SELECT_DATA_THRESHOLD)
                        && ((double) (collapsed_events[l].data2_max - collapsed_events[l].data2_min) / (double) (collapsed_events[l].time_max - collapsed_events[l].time_min) > MIDI_SELECT_RATIO_THRESHOLD)){
                            midi_connect_param(collapsed_events[l].device, collapsed_events[l].event, collapsed_events[l].data1);
                            n_recent_events = 0;
                        }
                    }
                }else{ // if(!active_param_source)
                    n_recent_events = 0;
                }
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
