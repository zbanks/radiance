#include "core/err.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "midi/midi.h"
#include "midi/config.h"
#include <string.h>

#include "core/config_macros.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif

#define CFGOBJ midi_map
#define CFGOBJ_PATH "midi/midi_map.def"

#include "core/config_gen_h.def"
#include "core/config_gen_c.def"

static param_state_t * midi_config_parse_param(char * pstr){
    if(!pstr || !*pstr) return NULL;
    char * sptr;
    char * target_str = strtok_r(pstr, " ", &sptr);
    char * target_idx_str = strtok_r(NULL, " ", &sptr);
    char * param_str = strtok_r(NULL, " ", &sptr);
    if(strcasecmp(target_str, "slot") == 0){
        int i = atoi(target_idx_str);
        if(i >= 0 && i < n_slots){
            int param_idx = 0;
            if(strcasecmp(param_str, "alpha") == 0){
                return &slots[i].alpha;
            }else if(sscanf(param_str, "%d", &param_idx) == 1){
                if(param_idx >= 0 && param_idx < N_MAX_PARAMS){
                    return &slots[i].param_states[param_idx];
                }
            }
        }
    }else if(strcasecmp(target_str, "slot") == 0){
        int i = atoi(target_idx_str);
        if(i >= 0 && i < n_signals){
            int param_idx = 0;
            if(sscanf(param_str, "%d", &param_idx) == 1){
                if(param_idx >= 0 && param_idx < signals[i].n_params){
                    return &signals[i].param_states[param_idx];
                }
            }
        }
    }
    ERROR("Invalid param string: '%s'\n", pstr);
    return NULL;
}

int midi_config_load(const char * filename, struct midi_controller * controllers, int n_controllers){
    struct midi_map map;
    midi_map_init(&map);

    if(midi_map_load(&map, filename))
        return -1;

    // Populate the `controllers` array with devices that are listed in the config file
    struct midi_controller * device;
    for(int i = 0; i < map.n_controllers; i++){
        if(!map.controllers[i].configured) continue;

        device = NULL;
        for(int j = 0; j < n_controllers; j++){
            if(!controllers[j].enabled){
                if(strcmp(map.controllers[i].name, controllers[j].name) == 0){
                    device = &controllers[j];
                    device->short_name = mystrdup(map.controllers[i].short_name);
                    device->enabled = 1;
                    break;
                }
            }
        }
        if(!device){
            WARN("No MIDI controller '%s' found.\n", map.controllers[i].name);
            continue;
        }

        // Iterate through Control Change connections
        for(int j = 0; j < map.controllers[i].n_ccs; j++){
            // Connect `device` CC#j to `map.ccs[j]`  
            char * param_str_copy = mystrdup(map.controllers[i].ccs[j]);
            char * sptr = NULL;
            for(char * param_str = strtok_r(param_str_copy, ";", &sptr); param_str != NULL; param_str = strtok_r(NULL, ";", &sptr)){
                param_state_t * param_state = midi_config_parse_param(param_str);
                if(param_state){
                    if(device->n_connections + 1 > N_MAX_MIDI_CONNECTIONS){
                        ERROR("More than %d MIDI connections specified for device %s\n", N_MAX_MIDI_CONNECTIONS, device->short_name);
                        return -1;
                    }

                    // Create connection to param_state
                    struct midi_connection * connection = &device->connections[device->n_connections++];
                    connection->event = MIDI_STATUS_CC;
                    connection->data1 = j;
                    connection->param_state = param_state;
                    connection->command_slot = NULL;
                }else{
                    ERROR("Unknown connection for CC%d:'%s'\n", j, map.controllers[i].ccs[j]);
                }
            }
            free(param_str_copy);
        }

        // Iterate through Note connections
        for(int j = 0; j < map.controllers[i].n_notes; j++){
            // Connect `device` NoteOn/Off/AT#j to `map.notes[j]`  
            if(device->n_connections + 3 > N_MAX_MIDI_CONNECTIONS){
                ERROR("More than %d MIDI connections specified for device %s\n", N_MAX_MIDI_CONNECTIONS, device->short_name);
                return -1;
            }

            char * param_str_copy = mystrdup(map.controllers[i].notes[j]);
            char * sptr = NULL;
            for(char * param_str = strtok_r(param_str_copy, ";", &sptr); param_str != NULL; param_str = strtok_r(NULL, ";", &sptr)){
                int slot_idx = 0;
                int event_idx = 0;
                printf("note %s\n", param_str);
                if(sscanf(param_str, "slot %d %d", &slot_idx, &event_idx) == 2){
                    if(slot_idx < 0 || slot_idx >= n_slots || event_idx < 0){
                        ERROR("Invalid event: '%s'\n", map.controllers[i].notes[j]);
                        continue;
                    }
                    printf("Making event %d %d\n", slot_idx, event_idx);

                    struct midi_connection * connection = &device->connections[device->n_connections++];
                    connection->event = MIDI_STATUS_NOTEON;
                    connection->data1 = j;
                    connection->param_state = NULL;
                    connection->command_slot = &slots[slot_idx];
                    connection->command_index = event_idx;
                    connection->command_status = STATUS_START;

                    connection = &device->connections[device->n_connections++];
                    connection->event = MIDI_STATUS_AFTERTOUCH;
                    connection->data1 = j;
                    connection->param_state = NULL;
                    connection->command_slot = &slots[slot_idx];
                    connection->command_index = event_idx;
                    connection->command_status = STATUS_CHANGE;

                    connection = &device->connections[device->n_connections++];
                    connection->event = MIDI_STATUS_NOTEOFF;
                    connection->data1 = j;
                    connection->param_state = NULL;
                    connection->command_slot = &slots[slot_idx];
                    connection->command_index = event_idx;
                    connection->command_status = STATUS_STOP;
                }
            }
        }
    }

    midi_map_del(&map);

    printf("Loaded configuration file: '%s'\n", filename);
    return 0;
}
