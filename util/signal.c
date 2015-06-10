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

// ---- Frequency Doubling ----

static inline double value_to_frequency(int zeroable, double value){
    if(zeroable)
        return power_zero_quantize_parameter(value);
    else
        return power_quantize_parameter(value);
}

void freq_init(struct freq_state * state, double initial_freq_val, int zeroable){
    state->freq = value_to_frequency(zeroable, initial_freq_val);
    state->phase = 0.;
    state->last_t = 0;
    state->zeroable  = zeroable;
}

int freq_update(struct freq_state * state, mbeat_t t, double target_freq_val){
    // Returns number of beat boundaries crossed since last update call
    int n_beats = 0;
    double new_freq = value_to_frequency(state->zeroable, target_freq_val);
#define BOSC(t, f) (MB2B(t % B2MB(1.0 / f)) * f) // TODO: this skews for freqs higher than 8
    if(new_freq != state->freq){
        if(new_freq == 0){
            // Switch to zero around the 50% period mark
            if((BOSC(t, state->freq) > 0.5) && (BOSC(state->last_t, state->freq) < 0.5)){
                state->freq = new_freq;
                state->phase = 0.5;
            }
        }else if(state->freq == 0){
            // Switch to new zero around the 50% period mark
            if((BOSC(t, new_freq) > 0.5) && (BOSC(state->last_t, new_freq) < 0.5)){
                state->freq = new_freq;
                state->phase = 0.5;
                // Set last_t to the time of the 50% crossing
                state->last_t += B2MB(0.5 - BOSC(state->last_t, new_freq));
            }
        }else{
            while(new_freq != state->freq){
                double next_freq;
                double big_freq, sml_freq;
                if(new_freq > state->freq){
                    next_freq = state->freq * 2.;
                    if(next_freq > new_freq) next_freq = new_freq;
                    big_freq = next_freq;
                    sml_freq = state->freq;
                }else{
                    next_freq = state->freq * 0.5;
                    if(next_freq < new_freq) next_freq = new_freq;
                    big_freq = state->freq;
                    sml_freq = next_freq;
                }
                double dp1 = BOSC(state->last_t, big_freq) - BOSC(state->last_t, sml_freq);
                double dp2 = BOSC(t, big_freq) - BOSC(t, sml_freq);
                double slope = big_freq - sml_freq;
                if(dp1 <= 0 && (dp2 > 0 || dp2 < dp1)){
                    // Crossing detected
                    // Determine when the crossing should have happened
                    double d = BOSC(state->last_t, next_freq) + dp1 / slope;
                    n_beats += (int) d;
                    state->last_t += B2MB(dp1 / slope);
                    state->phase = BOSC(state->last_t, next_freq);
                    state->freq = next_freq;
                    continue;
                }
                break;
            }
        }
    }

    if(state->freq == 0){
        state->phase = 0.5;
    }else{
        n_beats += (t - state->last_t) / B2MB(1.0 / state->freq);
        if(BOSC(state->last_t, state->freq) > BOSC(t, state->freq)) 
            n_beats++;

        state->phase = BOSC(t, state->freq);
    }

    state->last_t = t;
    return n_beats;
}
