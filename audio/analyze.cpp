#include "util/common.h"
#include "audio/analyze.h"
#include "util/config.h"
#include "util/err.h"
#include "util/math.h"
#include "time/timebase.h"
#include "main.h"
#include "BTrack/src/BTrack.h"
#include <fftw3.h>

std::unique_ptr<float[]> samp_queue;
static int samp_queue_ptr;
static fftw_plan plan;
std::unique_ptr<double[]> fft_in;
std::unique_ptr<fftw_complex[]> fft_out;
std::unique_ptr<double[]> spectrum;
std::unique_ptr<double[]> spectrum_lpf;
std::unique_ptr<GLfloat[]> spectrum_gl;
std::unique_ptr<int[]> spectrum_n;
std::unique_ptr<GLfloat[]> waveform_gl;
std::unique_ptr<GLfloat[]> waveform_beats_gl;
static int waveform_ptr;
static SDL_mutex * mutex;
static double audio_thread_hi;
static double audio_thread_mid;
static double audio_thread_low;
static double audio_thread_level;
static struct btrack btrack; 
std::unique_ptr<double[]> window;

// BTrack-based Timebase

/* TODO
static const struct time_source analyze_audio_time_source = {
    .name = "Audio Beat Tracking",
    .update = analyze_audio_time_update,
    .destroy = NULL,
};

double analyze_audio_time_update(struct time_source * source, long wall_ms) {
    ASSERT(source == analyze_audio_time_source);
    if (!init) return FP_NAN;

    // Naive; doesn't try to match phases or anything
    double beats_per_minute = btrack_get_bpm(&btrack);
    return wall_ms * MINUTES_PER_MILLISECOND * beats_per_minute;
}
*/

static double hann_window(int n) {
    // Hann window
    return 0.5 * (1 - cos(2 * M_PI * n / (config.audio.fft_length - 1)));
}

// Audio Analyze

void analyze_init() {
    // Audio processing
    samp_queue = std::make_unique<float[]>(config.audio.fft_length);
    fft_in = std::make_unique<double[]>(config.audio.fft_length);
    fft_out = std::make_unique<fftw_complex[]>(config.audio.fft_length / 2 + 1);
    spectrum = std::make_unique<double[]>(config.audio.spectrum_bins);
    spectrum_lpf = std::make_unique<double[]>(config.audio.spectrum_bins);
    spectrum_gl = std::make_unique<GLfloat[]>(config.audio.spectrum_bins);
    spectrum_n = std::make_unique<int[]>(config.audio.spectrum_bins);
    waveform_gl = std::make_unique<GLfloat[]>(config.audio.waveform_length * 8);
    waveform_beats_gl = std::make_unique<GLfloat[]>(config.audio.waveform_length * 8);
    window = std::make_unique<double[]>(config.audio.fft_length);
    for(int i=0; i<config.audio.fft_length; i++)
        window[i] = hann_window(i);

    samp_queue_ptr = 0;
    waveform_ptr = 0;
    plan = fftw_plan_dft_r2c_1d(config.audio.fft_length, fft_in.get(), fft_out.get(), FFTW_ESTIMATE);
    mutex = SDL_CreateMutex();
    if(mutex == NULL) FAIL("Could not create mutex: %s\n", SDL_GetError());
    audio_thread_hi = 0;
    audio_thread_mid = 0;
    audio_thread_low = 0;
    audio_thread_level = 0;
    //if (btrack_init(&btrack, config.audio.chunk_size, config.audio.fft_length, config.audio.sample_rate) != 0)
    if (btrack_init(&btrack, config.audio.chunk_size, 1024, config.audio.sample_rate) != 0)
        PFAIL("Could not initialize BTrack");
    //if (time_master_register_source(&analyze_audio_time_source) != 0)
    //    PFAIL("Could not register btrack time source");
}

void analyze_chunk(chunk_pt chunk) {
    // Add chunk samples to queue
    for(int i=0; i<config.audio.chunk_size; i++) {
        samp_queue[samp_queue_ptr] = chunk[i];
        samp_queue_ptr = (samp_queue_ptr + 1) % config.audio.fft_length;
    }

    // Window the data in queue and prepare it for FFTW
    for(int i=0; i<config.audio.fft_length; i++) {
        fft_in[i] = samp_queue[(samp_queue_ptr + i) % config.audio.fft_length] * window[i];
    }

    // Run the FFT
    fftw_execute(plan);

    // Convert to spectrum (log(freq))
    memset(spectrum.get(), 0, config.audio.spectrum_bins * sizeof spectrum[0]);
    memset(spectrum_n.get(), 0, config.audio.spectrum_bins * sizeof spectrum_n[0]);
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
    double level = 0;

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

    // Pass to BTrack. TODO: use already FFT'd values
    btrack_process_audio_frame(&btrack, chunk);

    double btrack_bpm = btrack_get_bpm(&btrack);
    time_update(TIME_SOURCE_AUDIO, TIME_SOURCE_EVENT_BPM, btrack_bpm);
    double ms_until_beat = btrack_get_time_until_beat(&btrack) * 1000.;
    time_update(TIME_SOURCE_AUDIO, TIME_SOURCE_EVENT_BEAT, ms_until_beat);

    static double beat_lpf = 0.0;
    if (btrack_beat_due_in_current_frame(&btrack)) {
        INFO("Beat; BPM=%lf", btrack_get_bpm(&btrack));
        if (time_master.beat_index % 4 == 0)
            beat_lpf = 1.0;
        else
            beat_lpf = 0.6;
    } else {
        beat_lpf *= 0.88;
    }

    // Convert to OpenGL floats
    if (SDL_LockMutex(mutex) != 0) FAIL("Could not lock mutex!");

    audio_thread_hi = hi / (1. - config.audio.hi_cutoff) * config.audio.waveform_gain;
    audio_thread_mid = mid / (config.audio.hi_cutoff - config.audio.low_cutoff) * config.audio.waveform_gain;
    audio_thread_low = low / config.audio.low_cutoff * config.audio.waveform_gain;

    level = audio_thread_hi;
    if(audio_thread_mid > level) level = audio_thread_mid;
    if(audio_thread_low > level) level = audio_thread_low;

    if(level > audio_thread_level) {
        level = level * config.audio.level_up_alpha + audio_thread_level * (1 - config.audio.level_up_alpha);
    } else {
        level = level * config.audio.level_down_alpha + audio_thread_level * (1 - config.audio.level_down_alpha);
    }

    audio_thread_level = level;

    for(int i=0; i<config.audio.spectrum_bins; i++) {
        spectrum_gl[i] = spectrum_lpf[i];
    }

    waveform_gl[waveform_ptr * 4 + 0] = audio_thread_hi;
    waveform_gl[waveform_ptr * 4 + 1] = audio_thread_mid;
    waveform_gl[waveform_ptr * 4 + 2] = audio_thread_low;
    waveform_gl[waveform_ptr * 4 + 3] = audio_thread_level;
    waveform_beats_gl[waveform_ptr * 4] = beat_lpf;

    memcpy(&waveform_gl[(config.audio.waveform_length + waveform_ptr) * 4], &waveform_gl[waveform_ptr * 4], 4 * sizeof waveform_gl[0]);
    memcpy(&waveform_beats_gl[(config.audio.waveform_length + waveform_ptr) * 4], &waveform_beats_gl[waveform_ptr * 4], 4 * sizeof waveform_beats_gl[0]);

    waveform_ptr = (waveform_ptr + 1) % config.audio.waveform_length;

    SDL_UnlockMutex(mutex);
}

// This is called from the OpenGL Thread
void analyze_render(GLuint tex_spectrum, GLuint tex_waveform, GLuint tex_waveform_beats) {
    if (SDL_LockMutex(mutex) != 0) FAIL("Could not lock mutex!");
    glBindTexture(GL_TEXTURE_1D,tex_spectrum);
    glTexSubImage1D(GL_TEXTURE_1D, 0,0,  config.audio.spectrum_bins, GL_RED, GL_FLOAT, spectrum_gl.get());
    glBindTexture(GL_TEXTURE_1D,tex_waveform);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0,config.audio.waveform_length, GL_RGBA, GL_FLOAT, &waveform_gl[waveform_ptr * 4]);
    glBindTexture(GL_TEXTURE_1D,tex_waveform_beats);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, config.audio.waveform_length, GL_RGBA, GL_FLOAT, &waveform_beats_gl[waveform_ptr * 4]);

    audio_hi = audio_thread_hi;
    audio_mid = audio_thread_mid;
    audio_low = audio_thread_low;
    audio_level = audio_thread_level;

    SDL_UnlockMutex(mutex);
}

void analyze_term() {
    SDL_DestroyMutex(mutex);
    fftw_destroy_plan(plan);
    btrack_del(&btrack);
    samp_queue = 0;
}

