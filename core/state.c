#include "core/err.h"
#include "core/slot.h"
#include "core/state.h"
#include "filters/filter.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "signals/signal.h"
#include "util/ini.h"

static int load_handler(void * user, const char * section, const char * name, const char * value){
    UNUSED(user);
    if(strcmp(section, "global") == 0){
        if(strcmp(name, "slots") == 0){
            if(atoi(value) > n_slots){
                printf("More slots in config file than in UI: %d vs %d.\n", atoi(value), n_slots);
            }
        }else if(strcmp(name, "signals") == 0){
            if(atoi(value) > n_signals){
                printf("More signals in config file than in UI: %d vs %d.\n", atoi(value), n_signals);
            }
        }
    }else if(memcmp(section, "signal_", 7) == 0){
        int i = atoi(section + 7);
        if(i < 0 || i > n_signals){
            printf("Invalid section: %s\n", section);
            return 0;
        }
        if(strcmp(name, "signal_name") == 0){
            if(strcmp(signals[i].name, value) != 0){
                printf("Unable to remap signals.\n");
                return 0;
            }
        }else if(memcmp(name, "param_", 6) == 0){
            int j = atoi(name + 6);
            if(j < 0 || j > signals[i].n_params){
                printf("Invalid name: %s [%s]%s=%s\n", name, section, name, value);
                return 0;
            }
            if(*value == '@'){
                if(param_state_connect_label(&signals[i].param_states[j], value+1)){
                    printf("Error connecting: %s %s %s\n", section, name, value);
                }
            }else{
                param_state_disconnect(&signals[i].param_states[j]);
                param_state_setq(&signals[i].param_states[j], atof(value));
            }
        }
    }else if(memcmp(section, "slot_", 5) == 0){
        int i = atoi(section + 5);
        if(i < 0 || i > n_slots){
            printf("Invalid section: %s\n", section);
            return 0;
        }
        if(strcmp(name, "pattern_name") == 0){
            for(int j = 0; j < n_patterns; j++){
                if(strcmp(patterns[j]->name, value) == 0){
                    pat_load(&slots[i], patterns[j]);
                    break;
                }
            }
        }else if(strcmp(name, "alpha") == 0){
            param_state_setq(&slots[i].alpha, atof(value));
        }else if(memcmp(name, "param_", 6) == 0){
            int j = atoi(name + 6);
            if(!slots[i].pattern || j < 0 || j > slots[i].pattern->n_params){
                printf("Invalid name: %s [%s]%s=%s\n", name, section, name, value);
                return 0;
            }
            if(*value == '@'){
                if(param_state_connect_label(&slots[i].param_states[j], value+1)){
                    printf("Error connecting: %s %s %s\n", section, name, value);
                }
            }else{
                param_state_disconnect(&slots[i].param_states[j]);
                param_state_setq(&slots[i].param_states[j], atof(value));
            }
        }else{
            printf("Invalid name: %s\n", name);
            return 0;
        }
    }else{
        printf("Invalid section: %s\n", section);
        return 0;
    }
    return 1;
}

int state_load(const char * filename){
    if(ini_parse(filename, load_handler, 0) < 0){
        printf("Unable to load layout configuration file: '%s'\n", filename);
        return -1;
    }
    printf("Loaded layout configuration file: '%s'\n", filename);
    return 0;
}

int state_save(const char * filename){
    FILE * stream = fopen(filename, "w");
    if(!stream) return -1;

    fprintf(stream, "[global]\n");
    fprintf(stream, "slots=%d\n", n_slots);
    fprintf(stream, "signals=%d\n", n_signals);

    for(int i = 0; i < n_slots; i++){
        fprintf(stream, "\n[slot_%d]\n", i);
        if(slots[i].pattern){
            fprintf(stream, "pattern_name=%s\n", slots[i].pattern->name);
            fprintf(stream, "alpha=%f\n", slots[i].alpha.value);
            for(int j = 0; j < slots[i].pattern->n_params; j++){
                if(slots[i].param_states[j].connected_output)
                    fprintf(stream, "param_%d=@%s\n", j, slots[i].param_states[j].connected_output->label);
                else
                    fprintf(stream, "param_%d=%f\n", j, slots[i].param_states[j].value);
            }
        }
    }

    for(int i = 0; i < n_signals; i++){
        fprintf(stream, "\n[signal_%d]\n", i);
        if(slots[i].pattern){
            fprintf(stream, "signal_name=%s\n", signals[i].name);
            for(int j = 0; j < signals[i].n_params; j++){
                if(signals[i].param_states[j].connected_output)
                    fprintf(stream, "param_%d=@%s\n", j, signals[i].param_states[j].connected_output->label);
                else
                    fprintf(stream, "param_%d=%f\n", j, signals[i].param_states[j].value);
            }
        }
    }

    fclose(stream);
    return -1;
}
