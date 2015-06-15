#include "core/err.h"
#include "core/state.h"
#include "core/slot.h"
#include "patterns/pattern.h"
#include "signals/signal.h"
#include "util/ini.h"
#include "midi/midi.h"
#include "midi/config.h"

#include "core/config_gen_c.def"

struct midi_conn_line {
    unsigned char channel;
    unsigned char data1;
    char short_name[128];
    struct midi_controller * device;
};

static int midi_config_parse_conn_line(struct midi_controller * controllers, int n_controllers, const char * conn_str, struct midi_conn_line * conn_line){
    if(!conn_line) return -1;
    if(!conn_str || !*conn_str) return -1; // conn_str is a null stirng
    memset(conn_line, 0, sizeof(struct midi_conn_line));

    if(sscanf(conn_str, "%127s %hhu %hhu", conn_line->short_name, &conn_line->channel, &conn_line->data1) != 3){
        ERROR("Invalid MIDI string '%s'. Format is 'name status data1' e.g. 'nk2 127 49'\n", conn_str);
        return -1;
    }
    if(conn_line->channel >= 16){
        ERROR("Invalid MIDI channel: %hhu.\n", conn_line->channel);
        return -1;
    }
    for(int i = 0; i < n_controllers; i++){
        if(controllers[i].enabled){
            if(strcmp(conn_line->short_name, controllers[i].short_name) == 0){
                conn_line->device = &controllers[i];
            }
        }
    }
    if(!conn_line->device){
        ERROR("No device with name '%s' initialized.\n", conn_line->short_name);
        return -1;
    }
    return 0;
}

static int midi_config_connect_param(struct midi_conn_line * conn, param_state_t * param_state){
    if(!param_state) return -1;
    if(!conn) return -1;

    if(conn->device->n_connections + 1 > N_MAX_MIDI_CONNECTIONS){
        ERROR("More than %d MIDI connections specified for device %s\n", N_MAX_MIDI_CONNECTIONS, conn->short_name);
        return -1;
    }

    struct midi_connection * connection = &conn->device->connections[conn->device->n_connections++];
    connection->event = conn->channel | MIDI_STATUS_CC;
    connection->data1 = conn->data1;
    connection->param_state = param_state;
    connection->slot_event.slot = NULL;

    return 0;
}

static int midi_config_connect_event(struct midi_conn_line * conn, slot_t * slot, int event_index){
    if(!conn) return -1;
    if(!slot) return -1;

    if(conn->device->n_connections + 3 > N_MAX_MIDI_CONNECTIONS){
        ERROR("More than %d MIDI connections specified for device %s\n", N_MAX_MIDI_CONNECTIONS, conn->short_name);
        return -1;
    }

    struct midi_connection * connection = &conn->device->connections[conn->device->n_connections++];
    connection->event = conn->channel | MIDI_STATUS_NOTEON;
    connection->data1 = conn->data1;
    connection->param_state = NULL;
    connection->slot_event.slot = slot;
    connection->slot_event.event.source = event_index + PATSRC_MIDI_0;
    connection->slot_event.event.event = PATEV_START;

    connection = &conn->device->connections[conn->device->n_connections++];
    connection->event = conn->channel | MIDI_STATUS_AFTERTOUCH;
    connection->data1 = conn->data1;
    connection->param_state = NULL;
    connection->slot_event.slot = slot;
    connection->slot_event.event.source = event_index + PATSRC_MIDI_0;
    connection->slot_event.event.event = PATEV_MIDDLE;

    connection = &conn->device->connections[conn->device->n_connections++];
    connection->event = conn->channel | MIDI_STATUS_NOTEOFF;
    connection->data1 = conn->data1;
    connection->param_state = NULL;
    connection->slot_event.slot = slot;
    connection->slot_event.event.source = event_index + PATSRC_MIDI_0;
    connection->slot_event.event.event = PATEV_END;
    
    return 0;
}

int midi_config_load(const char * filename, struct midi_controller * controllers, int n_controllers){
    struct midi_map map;
    // `midi_map` is the global instance containing defaults
    memcpy(&map, &midi_map, sizeof(struct midi_map));

    if(midi_map_load(&map, filename)){
        ERROR("Unable to parse MIDI map file: %s\n", filename);
        return -1;
    }

    // Populate the `controllers` array with devices that are listed in the config file
    struct midi_controller * device;
    for(int i = 0; i < map.n_controllers; i++){
        if(!map.controllers[i].configured) continue;

        device = NULL;
        for(int j = 0; j < n_controllers; j++){
            if(!controllers[j].enabled){
                if(strcmp(map.controllers[i].name, controllers[j].name) == 0){
                    device = &controllers[j];
                    device->short_name = map.controllers[i].short_name;
                    device->enabled = 1;
                    break;
                }
            }
        }
        if(!device){
            WARN("No MIDI controller '%s' found.\n", map.controllers[i].name);
            continue;
        }
    }

    // Iterate through slots
    for(int i = 0; i < n_slots && i < map.n_slots; i++){
        if(!map.slots[i].configured) continue;

        // Map alpha
        param_state_t * param_state = &slots[i].alpha;
        const char * conn_str = map.slots[i].alpha;
        struct midi_conn_line conn;
        if(!midi_config_parse_conn_line(controllers, n_controllers, conn_str, &conn)){
            midi_config_connect_param(&conn, param_state);
        }

        // Map Params
        for(int j = 0; j < map.slots[i].n_params; j++){
            if(map.slots[i].params[j].index >= N_MAX_PARAMS){
                ERROR("Invalid param number: %d (max %d)", map.slots[i].params[j].index, N_MAX_PARAMS);
                continue;
            }
            param_state = &slots[i].param_states[map.slots[i].params[j].index];
            conn_str = map.slots[i].params[j].param;
            if(midi_config_parse_conn_line(controllers, n_controllers, conn_str, &conn))
                continue;
            if(midi_config_connect_param(&conn, param_state))
                continue;
        }
 
        // Map Events
        for(int j = 0; j < map.slots[i].n_events; j++){
            conn_str = map.slots[i].events[j].event;
            if(midi_config_parse_conn_line(controllers, n_controllers, conn_str, &conn))
                continue;
            if(midi_config_connect_event(&conn, &slots[i], map.slots[i].events[j].index))
                continue;
        }
    }

    // Iterate through signals
    for(int i = 0; i < n_signals && i < map.n_signals; i++){
        if(!map.signals[i].configured) continue;

        // Map Params
        for(int j = 0; j < map.signals[i].n_params; j++){
            if(map.signals[i].params[j].index >= signals[i].n_params){
                ERROR("Invalid param number: %d (max %d)", map.slots[i].params[j].index, map.signals[i].n_params);
                continue;
            }
            param_state_t * param_state = &signals[i].param_states[map.signals[i].params[j].index];
            const char * conn_str = map.signals[i].params[j].param;
            struct midi_conn_line conn;
            if(midi_config_parse_conn_line(controllers, n_controllers, conn_str, &conn))
                continue;
            if(midi_config_connect_param(&conn, param_state))
                continue;
        }
    }

    printf("Loaded configuration file: '%s'\n", filename);
    return 0;
}
