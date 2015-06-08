#include "util/signal.h"
#include "util/math.h"
#include <math.h>
#include <stdio.h>

void ema_init(struct ema_state * state, double tau){
    state->out_1 = 0.;
    state->in_1 = 0.;
    state->t_1 = 0;
    state->tau = tau;
}

double ema_update(struct ema_state * state, mbeat_t t, double in){
    if(state->t_1 == 0) state->t_1 = t;
    // time should be monotonically increasing!
    if(state->t_1 > t) state->t_1 = t; 
    if(isnan(state->out_1)) state->out_1 = 0.;

    double dt = MB2B(t - state->t_1);
    double w, w2;
    if(state->tau > 1e-4){
        double k = dt / state->tau;
        w = exp(-k);
        w2 = (1 - w) / k;
    }else{
        w = 0.;
        w2 = 0.;
    }

    state->out_1 = state->out_1 * w + in * (1. - w2) + state->in_1 * (w2 - w);
    state->in_1 = in;
    state->t_1 = t;

    return state->out_1;
}

void dema_init(struct ema_state * state, double tau_rise, double tau_fall){
    state->out_1 = 0.;
    state->in_1 = 0.;
    state->t_1 = 0;
    state->tau_rise = tau_rise;
    state->tau_fall = tau_fall;
}

double dema_update(struct ema_state * state, mbeat_t t, double in){
    if(state->t_1 == 0) state->t_1 = t;
    // time should be monotonically increasing!
    if(state->t_1 > t) state->t_1 = t; 
    if(isnan(state->out_1)) state->out_1 = 0.;

    double tau;
    if(in > state->out_1){
        tau = state->tau_rise;
    }else{
        tau = state->tau_fall;
    }

    double dt = MB2B(t - state->t_1);
    double w;
    double w2;
    if(tau > 1e-4){
        double k = dt / tau;
        w = exp(-k);
        w2 = (1 - w) / k;
    }else{
        w = 0.;
        w2 = 0.;
    }

    state->out_1 = state->out_1 * w + in * (1. - w2) + state->in_1 * (w2 - w);
    state->in_1 = in;
    state->t_1 = t;

    return state->out_1;
}

// ---- Automatic Gain Control / Compressor ----

void agc_init(struct agc_state * state, double range_high, double range_low, double knee_high, double knee_low, double tau){

    // EMA for envelopes
    dema_init(&state->env_high, 0., tau);
    dema_init(&state->env_low, tau, 0.);

    state->range_high = range_high;
    state->range_low = range_low;

    state->knee_high = knee_high;
    state->knee_low = knee_low;
}

double agc_update(struct agc_state * state, mbeat_t t, double in){
    double env_in_high;
    double env_in_low;
    // Set knee_high <= knee_low to disable knee-ing
    if(state->knee_high > state->knee_low){
        env_in_high = MAX(in, state->knee_high);
        env_in_low = MIN(in, state->knee_low);
    }else{
        env_in_high = env_in_low = in;
    }

    double env_high = dema_update(&state->env_high, t, env_in_high);
    double env_low = dema_update(&state->env_low, t, env_in_low);

    // Scale input to [0, 1]
    double x = (in - env_low) / (env_high - env_low);

    // Scale & output
    return x * (state->range_high - state->range_low) + state->range_low;
}
