#include "core/err.h"
#include "core/slot.h"
#include "core/state.h"
#include "filters/filter.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "signals/signal.h"
#include "util/ini.h"
#include "util/color.h"

static int parse_param(const char * name, const char * value, param_state_t * params, int n_params){
    if(memcmp(name, "param_", 6) != 0){
        printf("Invalid call to parse_param: name='%s'\n", name);
        return -1;
    }
    int j = atoi(name + 6);
    if(j < 0 || j >= n_params){
        printf("Invalid name: %s=%s (idx=%d)\n", name, value, j);
        return -1;
    }
    if(*value == '@'){
        float min;
        float max;
        char sout[32];
        if(value[1] == '['){
            if(sscanf(value, "@[%f,%f]%31s", &min, &max, sout) == 3){
                if(param_state_connect_label(&params[j], sout)){
                    printf("Error connecting: %s %s\n", name, value);
                }
                param_state_set_range(&params[j], PARAM_VALUE_SCALED, min, max);
            }else{
                printf("Error parsing '%s'\n", value);
                return -1;
            }
        }else if(value[1] == '('){
            if(sscanf(value, "@(%f,%f)%31s", &min, &max, sout) == 3){
                if(param_state_connect_label(&params[j], sout)){
                    printf("Error connecting: %s %s\n", name, value);
                }
                param_state_set_range(&params[j], PARAM_VALUE_EXPANDED, min, max);
            }else{
                printf("Error parsing '%s'\n", value);
                return -1;
            }
        }else{
            if(param_state_connect_label(&params[j], value+1)){
                printf("Error connecting: %s %s\n", name, value);
            }
        }
    }else{
        param_state_disconnect(&params[j]);
        param_state_setq(&params[j], atof(value));
    }
    return 0;
}

static int dump_param(FILE * stream, param_state_t * param_state, parameter_t * parameter, int index){
    if(param_state->connected_output){
        char * sfmt = "param_%1$d=@%2$-31s";
        if(param_state->mode == PARAM_VALUE_SCALED){
            sfmt = "param_%1$d=@[%3$6f,%4$6f]%2$-18s";
        }else if(param_state->mode == PARAM_VALUE_EXPANDED){
            sfmt = "param_%1$d=@(%3$6f,%4$6f)%2$-18s";
        }
        fprintf(stream, sfmt, index, param_state->connected_output->label, param_state->min, param_state->max);
    }else{
        fprintf(stream, "param_%d=%-32f", index, param_state->value);
    }

    fprintf(stream, " ; %s",  parameter->name);
    char buf[32];
    if(param_state->connected_output || !parameter->val_to_str){
        fprintf(stream, "\n");
    }else{
        parameter->val_to_str(param_state->value, buf, sizeof(buf));
        fprintf(stream, " = %s\n", buf);
    }

    return 0;
}

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
        }else if(strcmp(name, "palette") == 0){
            for(int i = 0; i < n_colormaps; i++){
                if(strcmp(value, colormaps[i]->name) == 0){
                    colormap_set_global(colormaps[i]); 
                    break;
                }
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
            if(parse_param(name, value, signals[i].param_states, signals[i].n_params))
                return 0;
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
                    if(slots[i].pattern != patterns[j])
                        pat_load(&slots[i], patterns[j]);
                    break;
                }
            }
        }else if(strcmp(name, "colormap") ==  0){
            for(int j = 0; j < n_colormaps; j++){
                if(strcmp(value, colormaps[j]->name) == 0){
                    slots[i].colormap = colormaps[j];
                    break;
                }
            }
            if(strcmp(name, "GLOBAL") == 0){
                slots[i].colormap = NULL;
            }
        }else if(strcmp(name, "alpha") == 0){
            param_state_setq(&slots[i].alpha, atof(value));
        }else if(memcmp(name, "param_", 6) == 0){
            if(!slots[i].pattern){
                printf("No pattern for param: [%s] %s\n", section, name);
                return 0;
            }
            if(parse_param(name, value, slots[i].param_states, slots[i].pattern->n_params))
                return 0;
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
            if(slots[i].colormap)
                fprintf(stream, "colormap=%s\n", slots[i].colormap->name);
            else
                fprintf(stream, "colormap=GLOBAL\n");
            fprintf(stream, "alpha=%f\n", slots[i].alpha.value);
            for(int j = 0; j < slots[i].pattern->n_params; j++){
                dump_param(stream, &slots[i].param_states[j], &slots[i].pattern->parameters[j], j);
            }
        }
    }

    for(int i = 0; i < n_signals; i++){
        fprintf(stream, "\n[signal_%d]\n", i);
        if(slots[i].pattern){
            fprintf(stream, "signal_name=%s\n", signals[i].name);
            for(int j = 0; j < signals[i].n_params; j++){
                dump_param(stream, &signals[i].param_states[j], &signals[i].parameters[j], j);
            }
        }
    }

    fclose(stream);
    return 0;
}
