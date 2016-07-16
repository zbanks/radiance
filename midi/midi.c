#include <portmidi.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>

#include "util/err.h"
#include "util/math.h"
#include "util/config.h"
#include "midi/midi.h"
#include "midi/config.h"

#define MIDI_BUFFER_SIZE 256

static int midi_running;
static SDL_Thread* midi_thread;

static PmEvent events[MIDI_BUFFER_SIZE];

Uint32 midi_command_event = -1;

struct midi_controller * midi_controllers;
int n_midi_controllers;

PmError pm_errmsg(PmError err){
    ERROR("CAUGHT PM ERROR");
    return err;
}

int midi_refresh_devices(){
    PmError err;
    // Delete old midi controllers
    if(midi_controllers){
        for(int i = 0; i < n_midi_controllers; i++){
            if(midi_controllers[i].stream){
                err = Pm_Close(midi_controllers[i].stream);
                if(err != pmNoError) 
                    WARN("Could not close MIDI device: %s (%s); %s", 
                            midi_controllers[i].short_name, 
                            midi_controllers[i].name, 
                            Pm_GetErrorText(err));
            }
        }
        free(midi_controllers);
        midi_controllers = NULL;
    }

    int n = Pm_CountDevices();
    struct midi_controller * new_controllers = calloc(n, sizeof(struct midi_controller));
    if(!new_controllers) FAIL("Unable to malloc for MIDI controllers.");
    int n_new = 0;

    // Check for available devices
    for(PmDeviceID i = 0; i < n; i++) {
        const PmDeviceInfo* device = Pm_GetDeviceInfo(i);
        if(!device){
            ERROR("Unable to get device info for PM device %d", i);
            goto refresh_fail;
        }
        if(device->input) {
            INFO("Midi device %d %s", i, device->name);
            new_controllers[n_new].device_id = i;
            new_controllers[n_new].name = device->name;
            n_new++;
        }
    }
    // Read configuration from file
    if(midi_config_load(&midi_config, config.midi.config)){
        ERROR("Unable to read MIDI configuration file %s", config.midi.config);
        goto refresh_fail;
    }

    // Open corresponding PortMidiStream's
    for(int i = 0; i < n_new; i++) {
        int j;
        for (j = 0; j < midi_config.n_controllers; j++) {
            if (!midi_config.controllers[j].configured)
                continue;
            if (strcmp(midi_config.controllers[j].name, new_controllers[i].name) == 0)
                break;
        }
        if(j < midi_config.n_controllers) {
            new_controllers[i].enabled = true;
            new_controllers[i].short_name = midi_config.controllers[j].short_name;
            new_controllers[i].config = &midi_config.controllers[j];
            err = Pm_OpenInput(&new_controllers[i].stream, new_controllers[i].device_id, NULL, MIDI_BUFFER_SIZE, NULL, NULL);
            if(err != pmNoError){
                ERROR("Could not open MIDI device: %s", Pm_GetErrorText(err));
                new_controllers[i].device_id = 0;
                new_controllers[i].enabled = 0;
                new_controllers[i].stream= NULL;
                //goto refresh_fail; // Soft fail
            }else{
                INFO("Connected MIDI device: %s (%s)", new_controllers[i].short_name, new_controllers[i].name);
            }
        }
    }

    midi_controllers = new_controllers;
    n_midi_controllers = n_new;

    return 0;

refresh_fail:
    if(new_controllers){
        for(int i = 0; i < n_new; i++){
            if(new_controllers[i].stream){
                err = Pm_Close(new_controllers[i].stream);
                if(err != pmNoError) 
                    WARN("Could not close MIDI device: %s (%s); %s", 
                            new_controllers[i].short_name, 
                            new_controllers[i].name, 
                            Pm_GetErrorText(err));
            }
        }
        free(new_controllers);
    }
    return -1;
}

static int midi_check_errors(struct midi_controller * controller){
    if(!controller->stream) return 0;
    PmError err = Pm_HasHostError(controller->stream);
    if(err < 0){
        WARN("MIDI Host error: %s", Pm_GetErrorText(err));

        // Close stream  & disconnect 
        Pm_Close(controller->stream);
        controller->stream = 0;

        // Refresh devices (attempt to reconnect the device that we just lost) 
        // TODO
        //midi_refresh_devices();
        return 1;
    }
    return 0;
}

static int midi_run(void* args) {
    PmError err;

    err = Pm_Initialize();
    if(err != pmNoError) FAIL("Could not initialize PortMIDI: %s", Pm_GetErrorText(err));

    midi_refresh_devices();

    while(midi_running) {
        for(int i = 0; i < n_midi_controllers; i++) {
            struct midi_controller * controller = &midi_controllers[i];
            if(!controller->stream) continue;
            if(midi_check_errors(controller)) continue;

            int n = Pm_Read(controller->stream, events, MIDI_BUFFER_SIZE);
            if(n < 0){
                WARN("MIDI Read error: %s", Pm_GetErrorText(n));
                continue;
            }
            for(int j = 0; j < n; j++) {
                PmMessage m = events[j].message;

                unsigned char event = Pm_MessageStatus(m);
                unsigned char data1 = Pm_MessageData1(m);
                unsigned char data2 = Pm_MessageData2(m);

                //INFO("Device %d event %d %d %d %li", i, event, data1, data2, (long int) events[j].timestamp);
                int slot = -1;
                switch (event) {
                case MIDI_STATUS_CC:
                    if (data1 >= controller->config->n_ccs) break;
                    slot = controller->config->ccs[data1];
                    if (slot < 0) break;

                    float * event_data = calloc(1, sizeof *event_data);
                    if (event_data == NULL) MEMFAIL();
                    *event_data = (float) data2 / 127.0;

                    SDL_Event sdl_event;
                    sdl_event.type = midi_command_event;
                    sdl_event.user.code = slot;
                    sdl_event.user.data1 = event_data;
                    sdl_event.user.data2 = 0;
                    SDL_PushEvent(&sdl_event);
                    break;
                case MIDI_STATUS_NOTEON:
                case MIDI_STATUS_NOTEOFF:
                    if (data1 >= controller->config->n_notes) break;
                    slot = controller->config->notes[data1];
                    if (slot < 0) break;
                    break;
                }
            }
        }
        SDL_Delay(1); // TODO SDL rate limiting
    }

    for(int i = 0; i < n_midi_controllers; i++) {
        if(!midi_controllers[i].stream) continue;
        err = Pm_Close(midi_controllers[i].stream);
        if(err != pmNoError) FAIL("Could not close MIDI device: %s", Pm_GetErrorText(err));
    }

    err = Pm_Terminate();
    if(err != pmNoError) FAIL("Could not terminate PortMIDI: %s", Pm_GetErrorText(err));

    return 0;
}

void midi_start() {
    midi_command_event = SDL_RegisterEvents(1);

    midi_running = 1;
    midi_config_init(&midi_config);
    midi_refresh_devices();

    midi_thread = SDL_CreateThread(&midi_run, "Midi", 0);
    if(!midi_thread) FAIL("Could not create MIDI thread: %s",SDL_GetError());
}

void midi_stop() {
    midi_running = 0;
    midi_config_del(&midi_config);

    SDL_WaitThread(midi_thread, 0);
    INFO("MIDI thread stopped.");
}

