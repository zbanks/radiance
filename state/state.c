#include "core/err.h"
#include "core/slot.h"
#include "state/state.h"
#include "filters/filter.h"
#include "patterns/pattern.h"
#include "patterns/static.h"
#include "signals/signal.h"
#include "util/ini.h"
#include "util/color.h"

#include "core/config_macros.h"

#ifdef CFGOBJ
#undef CFGOBJ
#undef CFGOBJ_PATH
#endif

#define CFGOBJ state_data
#define CFGOBJ_PATH "state/state.def"

#define PALETTE struct colormap *
#define PALETTE_PARSE(x) _find_palette(x)
#define PALETTE_FORMAT(x) "%s", (x ? x->name : cm_global->name)
#define PALETTE_FREE(x) (void)(x)


static struct colormap * _find_palette(const char * palette_str){
    if(strcmp(palette_str, "Global") == 0){
        return NULL;
    }
    for(int i = 0; i < n_colormaps; i++){
        if(strcmp(palette_str, colormaps[i]->name) == 0){
            return colormaps[i];
        }
    }
    WARN("Invalid palette name '%s'\n", palette_str);
    return NULL;
}

#include "core/config_gen_h.def"
#include "core/config_gen_c.def"

int state_load(const char * filename){
    struct state_data state;
    state_data_init(&state); // Load with default state
    if(state_data_load(&state, filename))
        return -1;

    int found = 0;

    if(state.global.palette == NULL){
        ERROR("Global palette cannot be 'Global'\n");
        colormap_set_global(colormaps[0]);
    }else{
        colormap_set_global(state.global.palette);
    }

    // Configure slots
    for(int i = 0; i < n_slots && i < state.n_slots; i++){
        // Is there a pattern there?
        if(!state.slots[i].configured || strcmp(state.slots[i].pattern, "None") == 0){
            pat_unload(&slots[i]);
            continue;
        }

        // Load in pattern
        found = 0;
        for(int j = 0; j < n_patterns; j++){
            if(strcmp(state.slots[i].pattern, patterns[j]->name) == 0){
                if(patterns[j] != slots[i].pattern)
                    pat_load(&slots[i], patterns[j]);
                found = 1;
                break;
            }
        }
        if(!found){
            WARN("Invalid pattern name for slot %d: '%s'\n", i, state.slots[i].pattern);
            continue;
        }

        // Set palette
        slots[i].colormap = state.slots[i].palette;
        
        // Set alpha
        param_state_setq(&slots[i].alpha, state.slots[i].alpha);
        param_state_connect_label(&slots[i].alpha, state.slots[i].alpha_source);

        // Set params
        for(int j = 0; j < state.slots[i].n_param_sources; j++){
            if(state.slots[i].param_sources[j].index < N_MAX_PARAMS){
                param_state_connect_label(&slots[i].param_states[state.slots[i].param_sources[j].index], state.slots[i].param_sources[j].param_source);
            }else{
                WARN("Invalid parameter source number: %d\n", j);
            }
        }
        for(int j = 0; j < state.slots[i].n_params; j++){
            if(state.slots[i].params[j].index < N_MAX_PARAMS){
                param_state_setq(&slots[i].param_states[state.slots[i].params[j].index], state.slots[i].params[j].param);
            }else{
                WARN("Invalid parameter number: %d\n", j);
            }
        }
    }

    // Configure signals
    for(int i = 0; i < n_signals && i < state.n_signals; i++){
        // Set params
        for(int j = 0; j < state.signals[i].n_param_sources; j++){
            if(state.signals[i].param_sources[j].index < N_MAX_PARAMS){
                param_state_connect_label(&signals[i].param_states[state.signals[i].param_sources[j].index], state.signals[i].param_sources[j].param_source);
            }else{
                WARN("Invalid parameter source number: %d\n", j);
            }
        }
        for(int j = 0; j < state.signals[i].n_params; j++){
            if(state.signals[i].params[j].index < N_MAX_PARAMS){
                param_state_setq(&signals[i].param_states[state.signals[i].params[j].index], state.signals[i].params[j].param);
            }else{
                WARN("Invalid parameter number: %d\n", j);
            }
        }
    }
    state_data_del(&state);

    printf("Loaded state configuration file: '%s'\n", filename);
    return 0;
}

static int dump_param(FILE * stream, param_state_t * param_state, parameter_t * parameter, int i){
    if(param_state->connected_output){
        fprintf(stream, "param_source_%d=%-30s", i, param_state->connected_output->label);
    }else{
        fprintf(stream, "param_%d=%-32f", i, param_state->value);
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

int state_save(const char * filename){
    FILE * stream = fopen(filename, "w");
    if(!stream) return -1;

    fprintf(stream, "[" SIZE_SECTION_NAME "]\n");
    fprintf(stream, "n_slots=%d\n", n_slots);
    fprintf(stream, "n_signals=%d\n", n_signals);

    fprintf(stream, "\n[global]\n");
    fprintf(stream, "palette=%s\n", cm_global->name);

    for(int i = 0; i < n_slots; i++){
        fprintf(stream, "\n[slot_%d]\n", i);
        if(slots[i].pattern){
            fprintf(stream, "pattern=%s\n", slots[i].pattern->name);
            if(slots[i].colormap)
                fprintf(stream, "palette=%s\n", slots[i].colormap->name);
            else
                fprintf(stream, "palette=Global\n");
            fprintf(stream, "alpha=%f\n", slots[i].alpha.value);
            for(int j = 0; j < slots[i].pattern->n_params; j++){
                dump_param(stream, &slots[i].param_states[j], &slots[i].pattern->parameters[j], j);
            }
        }else{
            fprintf(stream, "pattern=None\n");
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
    printf("Wrote state to %s\n", filename);
    return 0;
}
