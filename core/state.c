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
                float min;
                float max;
                char sout[32];
                if(sscanf(value, "@[%f,%f]%31s", &min, &max, sout) == 3){
                    if(param_state_connect_label(&signals[i].param_states[j], sout)){
                        printf("Error connecting: %s %s %s\n", section, name, value);
                    }
                    signals[i].param_states[j].min = min;
                    signals[i].param_states[j].max = max;
                }else{
                    printf("Error parsing '%s'\n", value);
                    return 0;
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
                float min;
                float max;
                char sout[32];
                if(sscanf(value, "@[%f,%f]%31s", &min, &max, sout) == 3){
                    if(param_state_connect_label(&slots[i].param_states[j], sout)){
                        printf("Error connecting: %s %s %s\n", section, name, value);
                    }
                    slots[i].param_states[j].min = min;
                    slots[i].param_states[j].max = max;
                }else{
                    printf("Error parsing '%s'\n", value);
                    return 0;
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
                    fprintf(stream, "param_%d=@[%6f,%6f]%-18s", j,
                            slots[i].param_states[j].min,
                            slots[i].param_states[j].max,
                            slots[i].param_states[j].connected_output->label);
                else
                    fprintf(stream, "param_%d=%-32f", j, slots[i].param_states[j].value);
                fprintf(stream, " ; %s", slots[i].pattern->parameters[j].name);
                char buf[32];
                if(slots[i].param_states[j].connected_output || !slots[i].pattern->parameters[j].val_to_str){
                    fprintf(stream, "\n");
                }else{
                    slots[i].pattern->parameters[j].val_to_str(slots[i].param_states[j].value, buf, sizeof(buf));
                    fprintf(stream, " = %s\n", buf);
                }
            }
        }
    }

    for(int i = 0; i < n_signals; i++){
        fprintf(stream, "\n[signal_%d]\n", i);
        if(slots[i].pattern){
            fprintf(stream, "signal_name=%s\n", signals[i].name);
            for(int j = 0; j < signals[i].n_params; j++){
                //int padding = 32;
                if(signals[i].param_states[j].connected_output)
                    fprintf(stream, "param_%d=@[%6f,%6f]%-18s", j,
                            signals[i].param_states[j].min,
                            signals[i].param_states[j].max,
                            signals[i].param_states[j].connected_output->label);
                else
                    fprintf(stream, "param_%d=%-32f", j, signals[i].param_states[j].value);
                fprintf(stream, " ; %s",  slots[i].pattern->parameters[j].name);
                char buf[32];
                if(signals[i].param_states[j].connected_output || !signals[i].parameters[j].val_to_str){
                    fprintf(stream, "\n");
                }else{
                    signals[i].parameters[j].val_to_str(signals[i].param_states[j].value, buf, sizeof(buf));
                    fprintf(stream, " = %s\n", buf);
                }
            }
        }
    }

    fclose(stream);
    return 0;
}
