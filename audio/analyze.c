#include "audio/analyze.h"
#include "util/config.h"
#include "util/err.h"
#include <fftw3.h>
#define  M_PI 3.14159265358979323846
#include <math.h>

static float * samp_queue;
static int samp_queue_ptr;
static fftw_plan plan;
static double * fft_in;
static fftw_complex * fft_out;
static double * spectrum;
static double * spectrum_lpf;
static GLfloat * spectrum_gl;
static int * spectrum_n;
static GLfloat * waveform_gl;
static int waveform_ptr;

void analyze_init() {
    // Audio processing
    samp_queue = calloc(config.audio.fft_length, sizeof *samp_queue);
    if(samp_queue == NULL) MEMFAIL();
    fft_in = calloc(config.audio.fft_length, sizeof *fft_in);
    if(fft_in == NULL) MEMFAIL();
    fft_out = calloc(config.audio.fft_length / 2 + 1, sizeof *fft_out);
    if(fft_out == NULL) MEMFAIL();
    spectrum = calloc(config.audio.spectrum_bins, sizeof *spectrum);
    if(spectrum == NULL) MEMFAIL();
    spectrum_lpf = calloc(config.audio.spectrum_bins, sizeof *spectrum_lpf);
    if(spectrum_lpf == NULL) MEMFAIL();
    spectrum_gl = calloc(config.audio.spectrum_bins, sizeof *spectrum_gl);
    if(spectrum_gl == NULL) MEMFAIL();
    spectrum_n = calloc(config.audio.spectrum_bins, sizeof *spectrum_n);
    if(spectrum_n == NULL) MEMFAIL();
    waveform_gl = calloc(config.audio.waveform_length * 8, sizeof *waveform_gl);
    if(waveform_gl == NULL) MEMFAIL();
    samp_queue_ptr = 0;
    waveform_ptr = 0;
    plan = fftw_plan_dft_r2c_1d(config.audio.fft_length, fft_in, fft_out, FFTW_ESTIMATE);
}

static double window(int n) {
    // Hann window
    return 0.5 * (1 - cos(2 * M_PI * n / (config.audio.fft_length - 1)));
}

void analyze_chunk(chunk_pt chunk) {
    // Add chunk samples to queue
    for(int i=0; i<config.audio.chunk_size; i++) {
        samp_queue[samp_queue_ptr] = chunk[i];
        samp_queue_ptr = (samp_queue_ptr + 1) % config.audio.fft_length;
    }

    // Window the data in queue and prepare it for FFTW
    for(int i=0; i<config.audio.fft_length; i++) {
        fft_in[i] = samp_queue[(samp_queue_ptr + i) % config.audio.fft_length] * window(i);
    }

    // Run the FFT
    fftw_execute(plan);

    // Convert to spectrum (log(freq))
    memset(spectrum, 0, config.audio.spectrum_bins * sizeof *spectrum);
    memset(spectrum_n, 0, config.audio.spectrum_bins * sizeof *spectrum_n);
    double bin_factor = config.audio.spectrum_bins / log(config.audio.fft_length / 2);
    for(int i=1; i<config.audio.fft_length / 2; i++) {
        int bin = (int)(log(i) * bin_factor);
        spectrum[bin] += fft_out[i][0] * fft_out[i][0] + fft_out[i][1] * fft_out[i][1];
        spectrum_n[bin]++;
    }

    // Convert to spectrum (log(power))
    for(int i=0; i<config.audio.spectrum_bins; i++) {
        if(spectrum_n[i] == 0) {
            spectrum[i] = spectrum[i - 1];
        } else {
            spectrum[i] = config.audio.spectrum_gain * (log1p(spectrum[i]) - config.audio.spectrum_offset);
            if(spectrum[i] < 0) spectrum[i] = 0;
            if(spectrum[i] > 1) spectrum[i] = 1;
        }
    }

    // Diode-LPF and tally up hi, mid, low
    double hi = 0;
    double mid = 0;
    double low = 0;

    for(int i=0; i<config.audio.spectrum_bins; i++) {
        if(spectrum[i] > spectrum_gl[i]) {
            spectrum_lpf[i] = spectrum[i] * config.audio.spectrum_up_alpha + spectrum_lpf[i] * (1 - config.audio.spectrum_up_alpha);
        } else {
            spectrum_lpf[i] = spectrum[i] * config.audio.spectrum_down_alpha + spectrum_lpf[i] * (1 - config.audio.spectrum_down_alpha);
        }
        double freq_frac = (double)i / config.audio.spectrum_bins;
        if(freq_frac < config.audio.low_cutoff) {
            low += spectrum_lpf[i];
        } else if(freq_frac > config.audio.hi_cutoff) {
            hi += spectrum_lpf[i];
        } else {
            mid += spectrum_lpf[i];
        }
    }

    hi = hi / (1. - config.audio.hi_cutoff) * config.audio.waveform_gain;
    mid = mid / (config.audio.hi_cutoff - config.audio.low_cutoff) * config.audio.waveform_gain;
    low = low / config.audio.low_cutoff * config.audio.waveform_gain;

    // Convert to OpenGL floats
    // TODO lock here
    for(int i=0; i<config.audio.spectrum_bins; i++) {
        spectrum_gl[i] = spectrum_lpf[i];
    }

    waveform_gl[waveform_ptr * 4] = hi;
    waveform_gl[waveform_ptr * 4 + 1] = mid;
    waveform_gl[waveform_ptr * 4 + 2] = low;
    waveform_gl[(config.audio.waveform_length + waveform_ptr) * 4] = hi;
    waveform_gl[(config.audio.waveform_length + waveform_ptr) * 4 + 1] = mid;
    waveform_gl[(config.audio.waveform_length + waveform_ptr) * 4 + 2] = low;
    waveform_ptr = (waveform_ptr + 1) % config.audio.waveform_length;

    // TODO unlock here
}

// This is called from the OpenGL Thread
void analyze_render(GLuint tex_spectrum, GLuint tex_waveform) {
    // TODO lock here
    glBindTexture(GL_TEXTURE_1D, tex_spectrum);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, config.audio.spectrum_bins, 0, GL_RED, GL_FLOAT, spectrum_gl);
    glBindTexture(GL_TEXTURE_1D, tex_waveform);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, config.audio.waveform_length, 0, GL_RGBA, GL_FLOAT, &waveform_gl[waveform_ptr * 4]);
    glBindTexture(GL_TEXTURE_1D, 0);
    // TODO unlock here
}

void analyze_term() {
    fftw_destroy_plan(plan);
    free(samp_queue);
    free(fft_in);
    free(fft_out);
    free(spectrum);
    free(spectrum_lpf);
    free(spectrum_n);
    samp_queue = 0;
}
