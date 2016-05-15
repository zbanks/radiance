#include <math.h>

#include "patterns/pattern.h"
#include "signals/signal.h"
#include "util/siggen.h"
#include "util/math.h"
#include "util/signal.h"
#include "ui/graph.h"
#include "core/time.h"
#include "core/err.h"

#define N_SIGNALS 5

typedef struct
{
    struct freq_state freq_state;
    enum osc_type type;
} inp_lfo_state_t;

enum inp_lfo_param_names {
    LFO_TYPE,
    LFO_FREQ,

    N_LFO_PARAMS
};

parameter_t inp_lfo_parameters[N_LFO_PARAMS] = {
    [LFO_TYPE] = {
        .name = "Type",
        .default_val = 0,
        .val_to_str = osc_quantize_parameter_label,
    },
    [LFO_FREQ] = {
        .name = "Freq",
        .default_val = 0.5,
        .val_to_str = power_quantize_parameter_label,
    },
};

void inp_lfo_init(signal_t * signal){
    inp_lfo_state_t * state = signal->state = malloc(sizeof(inp_lfo_state_t));
    if(!signal->state) FAIL("Could not allocate LFO signal state\n");

    state->type = OSC_SINE;
    freq_init(&state->freq_state, 0.5, 0);
}

void inp_lfo_update(signal_t * signal, mbeat_t t){
    inp_lfo_state_t * state = (inp_lfo_state_t *) signal->state;
    if(!state) return;

    freq_update(&state->freq_state, t,  param_state_get(&signal->param_states[LFO_FREQ]));

    state->type = quantize_parameter(osc_quant_labels, param_state_get(&signal->param_states[LFO_TYPE]));
    param_output_set(&signal->output, osc_fn_gen(state->type, state->freq_state.phase));
}

void inp_lfo_del(signal_t * signal){
    if(!signal->state) return;
    free(signal->state);
    signal->state = 0;
}

typedef struct
{
    struct ema_state ema;
} inp_lpf_state_t;

enum inp_lpf_param_names {
    LPF_INPUT,
    LPF_RISE,
    LPF_FALL,

    N_LPF_PARAMS
};

parameter_t inp_lpf_parameters[N_LPF_PARAMS] = {
    [LPF_INPUT] = {
        .name = "Input",
        .default_val = 0,
        .val_to_str = float_to_string,
    },
    [LPF_RISE] = {
        .name = "Rise",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [LPF_FALL] = {
        .name = "Fall",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

void inp_lpf_init(signal_t * signal){
    inp_lpf_state_t * state = signal->state = malloc(sizeof(inp_lpf_state_t));
    if(!signal->state) return;

    dema_init(&state->ema, signal->parameters[LPF_RISE].default_val, signal->parameters[LPF_FALL].default_val);
}

void inp_lpf_update(signal_t * signal, mbeat_t t){
    inp_lpf_state_t * state = (inp_lpf_state_t *) signal->state;
    if(!state) return;
    float x = param_state_get(&signal->param_states[LPF_INPUT]);
    float rise = param_state_get(&signal->param_states[LPF_RISE]);
    float fall = param_state_get(&signal->param_states[LPF_FALL]);

    dema_set_tau(&state->ema, rise, fall);
    float out = dema_update(&state->ema, t, x);

    param_output_set(&signal->output, out);
}

void inp_lpf_del(signal_t * signal){
    if(!signal->state) return;
    free(signal->state);
    signal->state = 0;
}

typedef struct
{
    struct agc_state agc_state;
} inp_agc_state_t;

enum inp_agc_param_names {
    AGC_INPUT,
    AGC_MAX,
    AGC_MIN,
    AGC_DECAY,

    N_AGC_PARAMS
};

parameter_t inp_agc_parameters[N_AGC_PARAMS] = {
    [AGC_INPUT] = {
        .name = "Input",
        .default_val = 0,
        .val_to_str = float_to_string,
    },
    [AGC_MAX] = {
        .name = "Max (1 -> x)",
        .default_val = 1.,
        .val_to_str = float_to_string,
    },
    [AGC_MIN] = {
        .name = "Min (0 -> x)",
        .default_val = 0.,
        .val_to_str = float_to_string,
    },
    [AGC_DECAY] = {
        .name = "AGC Decay",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

void inp_agc_init(signal_t * signal){
    inp_agc_state_t * state = signal->state = malloc(sizeof(inp_agc_state_t));
    if(!signal->state) return;

    agc_init(&state->agc_state, 1.0, 0.0, 0, 0, 0.5);

}

void inp_agc_update(signal_t * signal, mbeat_t t){
    if(!signal->state) return;
    inp_agc_state_t * state = (inp_agc_state_t *) signal->state;

    float x = param_state_get(&signal->param_states[AGC_INPUT]);
    float min = param_state_get(&signal->param_states[AGC_MIN]);
    float max = param_state_get(&signal->param_states[AGC_MAX]);
    float a = param_state_get(&signal->param_states[AGC_DECAY]);

    agc_set_tau(&state->agc_state, a * 8.);
    agc_set_range(&state->agc_state, max, min);
    float y = agc_update(&state->agc_state, t, x);

    param_output_set(&signal->output, y);
}

void inp_agc_del(signal_t * signal){
    if(!signal->state) return;
    free(signal->state);
    signal->state = 0; 
}

#define QTL_MAXSIZE 150

typedef struct
{
    mbeat_t last_t;
    float state[QTL_MAXSIZE];
} inp_qtl_state_t;

enum inp_qtl_param_names {
    QTL_INPUT,
    QTL_QUANT,
    QTL_SIZE,

    N_QTL_PARAMS
};

parameter_t inp_qtl_parameters[N_QTL_PARAMS] = {
    [QTL_INPUT] = {
        .name = "Input",
        .default_val = 0,
        .val_to_str = float_to_string,
    },
    [QTL_QUANT] = {
        .name = "Quantile",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
    [QTL_SIZE] = {
        .name = "Size",
        .default_val = 0.5,
        .val_to_str = float_to_string,
    },
};

void inp_qtl_init(signal_t * signal){
    inp_qtl_state_t * state = signal->state = malloc(sizeof(inp_qtl_state_t));
    if(!signal->state) return;

    memset(state->state, 0, QTL_MAXSIZE * sizeof(float));
    state->last_t= 0;
}

static int _fcmp(const void * a, const void * b){
    return *(const float *)a > *(const float *)b;
}

void inp_qtl_update(signal_t * signal, mbeat_t t){
    static float istate[QTL_MAXSIZE];
    if(!signal->state) return;
    inp_qtl_state_t * state = (inp_qtl_state_t *) signal->state;

    float x = param_state_get(&signal->param_states[QTL_INPUT]);
    size_t s = param_state_get(&signal->param_states[QTL_SIZE]) * QTL_MAXSIZE;
    s = MAX(MIN(s, QTL_MAXSIZE - 1), 0);
    size_t q = param_state_get(&signal->param_states[QTL_QUANT]) * s;
    q = MAX(MIN(q, s), 0);

    if(state->last_t + 10 < t){
        if(state->last_t + 11 < t){
            state->last_t = t;
        }else{
            return;
        }
    }else{
        state->last_t += 10;
    }
    
    memmove(state->state+1, state->state, (QTL_MAXSIZE-1) * sizeof(float));
    state->state[0] = x;
    memcpy(istate, state->state, s * sizeof(float));
    qsort(istate, s, sizeof(float), &_fcmp);
    param_output_set(&signal->output, istate[q]);
}

void inp_qtl_del(signal_t * signal){
    if(!signal->state) return;
    free(signal->state);
    signal->state = 0; 
}

int n_signals = N_SIGNALS;
signal_t signals[N_SIGNALS] = {
    {
        .name = "LFO 1",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = N_LFO_PARAMS,
        .parameters = inp_lfo_parameters,
        .color = {1.0, 0.0, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {255, 0, 0, 255},
            .label_color = {255, 0, 0, 255},
            .label = "LFO1"
        },
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    },
    {
        .name = "LFO 2",
        .type = SIGNAL_LFO,
        .default_val = 0.5,
        .n_params = N_LFO_PARAMS,
        .parameters = inp_lfo_parameters,
        .color = {0.9, 0.3, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {220, 100, 0, 255},
            .label_color = {220, 100, 0, 255},
            .label = "LFO2"
        },
        .init = inp_lfo_init,
        .update = inp_lfo_update,
        .del = inp_lfo_del,
    },
    {
        .name = "LPF 1",
        .type = SIGNAL_LPF,
        .default_val = 0.5,
        .n_params = N_LPF_PARAMS,
        .parameters = inp_lpf_parameters,
        .color = {0.9, 0.9, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {240, 240, 0, 255},
            .label_color = {240, 240, 0, 255},
            .label = "LPF1"
        },
        .init = inp_lpf_init,
        .update = inp_lpf_update,
        .del = inp_lpf_del,
    },
    {
        .name = "AGC 1",
        .type = SIGNAL_AGC,
        .default_val = 0.5,
        .n_params = N_AGC_PARAMS,
        .parameters = inp_agc_parameters,
        .color = {0.1, 0.9, 0.0, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {25, 240, 0, 255},
            .label_color = {25, 240, 0, 255},
            .label = "AGC1"
        },
        .init = inp_agc_init,
        .update = inp_agc_update,
        .del = inp_agc_del,
    },
    /*
    {
        .name = "AGC 2",
        .type = SIGNAL_AGC,
        .default_val = 0.5,
        .n_params = N_AGC_PARAMS,
        .parameters = inp_agc_parameters,
        .color = {0.0, 0.8, 0.8, 0.0},
        .output = {
            .value = 0.0,
            .handle_color = {0, 220, 220, 255},
            .label_color = {0, 220, 220, 255},
            .label = "AGC"
        },
        .init = inp_agc_init,
        .update = inp_agc_update,
        .del = inp_agc_del,
    },
    */
    {
        .name = "Quantile 1",
        .type = SIGNAL_QTL,
        .default_val = 0.5,
        .n_params = N_QTL_PARAMS,
        .parameters = inp_qtl_parameters,
        .color = {0.0, 0.8, 0.8, 0.0},
        .output = {
            .handle_color = {0, 220, 220, 255},
            .label_color = {0, 220, 220, 255},
            .label = "QTL1"
        },
        .init = inp_qtl_init,
        .update = inp_qtl_update,
        .del = inp_qtl_del,
    },
};

void update_signals(mbeat_t t) {
    for(int i=0; i < n_signals; i++)
    {
        signals[i].update(&signals[i], t);
    }
}


void signal_start(){
    for(int i = 0; i < n_signals; i++){
        graph_create_signal(&signals[i].graph_state);
        param_output_init(&signals[i].output, 0.);

        signals[i].param_states = malloc(sizeof(param_state_t) * signals[i].n_params);
        if(!signals[i].param_states)
            FAIL("Unable to allocate space for signal params.\n");
        for(int j = 0; j < signals[i].n_params; j++){
            param_state_init(&signals[i].param_states[j], signals[i].parameters[j].default_val);
        }

        signals[i].init(&signals[i]);
    }
}

void signal_stop(){
    for(int i = 0; i < n_signals; i++){
        signals[i].del(&signals[i]);
        for(int j = 0; j < signals[i].n_params; j++){
            param_state_disconnect(&signals[i].param_states[j]);
        }
        free(signals[i].param_states);
        param_output_free(&signals[i].output);
        graph_remove(&signals[i].graph_state);
    }
    printf("Signals thread stopped.\n");
}
