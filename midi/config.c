#include "core/err.h"
#include "core/state.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "util/ini.h"
#include "midi/midi.h"

struct midi_config_parse_data {
    struct midi_controller * controllers;
    int n_controllers;
};

#define HANDLER_SUCCESS 1
#define HANDLER_ERROR 0

char * strdup(const char * str){
    size_t l = strlen(str) + 1;
    char * s = malloc(l);
    memcpy(s, str, l);
    return s;
}

static int midi_ini_handler(void * user, const char * section, const char * name, const char * value){
    struct midi_config_parse_data * data = (struct midi_config_parse_data *) user;
    struct midi_controller * device = NULL;

    if(strcmp(section, "controllers") == 0){
        for(int i = 0; i < data->n_controllers; i++){
            if(strcmp(value, data->controllers[i].name) == 0){
                if(!data->controllers[i].enabled){
                    device = &data->controllers[i];
                    device->short_name = strdup(name);
                    device->enabled = 1;
                    return HANDLER_SUCCESS;
                }
            }
        }
        WARN("No MIDI controller '%s' found.\n", value);
    }else{
        param_state_t * param_state = NULL;
        slot_t * slot = NULL;
        int event_base = 0;
        if(memcmp(section, "slot_", 5) == 0){
            int i = atoi(section + 5);
            if(i < 0 || i > n_slots){
                ERROR("Invalid section: %s\n", section);
                return HANDLER_ERROR;
            }
            if(memcmp(name, "event_", 6) == 0){
                int j = atoi(name + 6);
                if(j < 0 || j > (PATSRC_MAX - PATSRC_MIDI_0)){
                    ERROR("Invalid event number %d\n", j);
                    return HANDLER_ERROR;
                }
                event_base = j + PATSRC_MIDI_0;
                slot = &slots[i];
            }else if(memcmp(name, "param_", 6) == 0){
                int j = atoi(name + 6);
                if(j < 0 || j > N_MAX_PARAMS){
                    ERROR("Invalid param number %d\n", j);
                    return HANDLER_ERROR;
                }
                param_state = &slots[i].param_states[j];
            }else if(strcmp(name, "alpha") == 0){
                param_state = &slots[i].alpha;
            }
        }else if(memcmp(section, "signal_", 7) == 0){
            int i = atoi(section + 7);
            if(i < 0 || i > n_signals){
                ERROR("Invalid section: %s\n", section);
                return HANDLER_ERROR;
            }
            if(memcmp(name, "param_", 6) == 0){
                int j = atoi(name + 6);
                if(j < 0 || j > N_MAX_PARAMS){
                    ERROR("Invalid param number %d\n", j);
                    return HANDLER_ERROR;
                }
                param_state = &signals[i].param_states[j];
            }
        }
        if(!param_state && !event_base){
            ERROR("Invalid section: %s %s\n", section, name);
            return HANDLER_ERROR;
        }
        unsigned char channel, data1;
        char short_name[127];
        if(sscanf(value, "%127s %hhu %hhu", short_name, &channel, &data1) != 3){
            ERROR("Invalid MIDI string '%s'. Format is 'name status data1' e.g. 'nk2 127 49'\n", value);
            return HANDLER_ERROR;
        }
        if(channel >= 16){
            ERROR("Invalid MIDI channel: %hhu.\n", channel);
            return HANDLER_ERROR;
        }
        for(int k = 0; k < data->n_controllers; k++){
            if(data->controllers[k].enabled){
                if(strcmp(short_name, data->controllers[k].short_name) == 0){
                    device = &data->controllers[k];
                }
            }
        }
        if(!device){
            ERROR("No device with name '%s' initialized.\n", short_name);
            return HANDLER_ERROR;
        }
        int new_connections = event_base ? 3 : 1;
        if(device->n_connections + new_connections > N_MAX_MIDI_CONNECTIONS){
            ERROR("More than %d MIDI connections specified for device %s\n", N_MAX_MIDI_CONNECTIONS, short_name);
            return HANDLER_ERROR;
        }
        if(param_state){
            struct midi_connection * connection = &device->connections[device->n_connections++];
            connection->event = channel | MIDI_STATUS_CC;
            connection->data1 = data1;
            connection->param_state = param_state;
            connection->slot_event.slot = NULL;
        } else {
            struct midi_connection * connection = &device->connections[device->n_connections++];
            connection->event = channel | MIDI_STATUS_NOTEON;
            connection->data1 = data1;
            connection->param_state = NULL;
            connection->slot_event.slot = slot;
            connection->slot_event.event.source = event_base;
            connection->slot_event.event.event = PATEV_START;

            connection = &device->connections[device->n_connections++];
            connection->event = channel | MIDI_STATUS_AFTERTOUCH;
            connection->data1 = data1;
            connection->param_state = NULL;
            connection->slot_event.slot = slot;
            connection->slot_event.event.source = event_base;
            connection->slot_event.event.event = PATEV_MIDDLE;

            connection = &device->connections[device->n_connections++];
            connection->event = channel | MIDI_STATUS_NOTEOFF;
            connection->data1 = data1;
            connection->param_state = NULL;
            connection->slot_event.slot = slot;
            connection->slot_event.event.source = event_base;
            connection->slot_event.event.event = PATEV_END;
        }
    }
    return HANDLER_SUCCESS;
}

int midi_config_load(const char * filename, struct midi_controller * controllers, int n_controllers){
    struct midi_config_parse_data parse_data = {
        .controllers = controllers,
        .n_controllers = n_controllers,
    };
    if(ini_parse(filename, midi_ini_handler, &parse_data) < 0){
        printf("Unable to load midi configuration file: '%s'\n", filename);
        return -1;
    }
    printf("Loaded configuration file: '%s'\n", filename);
    return 0;
}
