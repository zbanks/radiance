#include "Audio.h"

#include <QDebug>
#include <numeric>
#include <algorithm>
#include <utility>
#include <portaudio.h>
#include <cmath>

#include "Timebase.h"

const int FrameRate = 44100;
const int ChunkSize = 512;
const int FFTLength = 2048;
const int SpectrumBins = 100;
const int WaveformLength = 512;
const float SpectrumUpAlpha = 0.5;
const float SpectrumDownAlpha = 0.2;
const float SpectrumGain = 0.1;
const float SpectrumOffset = 0.1;
const float LowCutoff = 0.3;
const float HiCutoff = 0.7;
const float WaveformGain = 0.005;
const float LevelUpAlpha = 0.9;
const float LevelDownAlpha = 0.01;

Audio::Audio(Timebase *timebase)
    : m_timebase(timebase)
{
    setObjectName("AudioThread");

    // Audio processing
    for(int i=0; i<FFTLength; i++)
        window[i] = hannWindow(i);

    //if (btrack_init(&btrack, chunk_size, FFTLength, sample_rate) != 0)
    if(btrack_init(&btrack, ChunkSize, 1024, FrameRate) != 0)
        throw std::runtime_error("Could not initialize BTrack");
    //if (time_master_register_source(&analyze_audio_time_source) != 0)
    //    PFAIL("Could not register btrack time source");

    start();
}

Audio::~Audio()
{
    quit();
    wait();
    if(m_plan)
        fftwf_destroy_plan(m_plan);
    m_plan = 0;
    btrack_del(&btrack);
    /* TODO: Figure this out.
    fftw_destroy_plan(plan);
    btrack_del(&btrack);
    delete chunk;
    chunk = 0;
    delete sampQueue;
    sampQueue = 0;
    delete fftIn;
    fftIn = 0;
    delete fftOut;
    fftOut = 0;
    delete spectrum;
    spectrum = 0;
    delete spectrumLPF;
    spectrumLPF = 0;
    delete spectrumGL;
    spectrumGL = 0;
    delete waveformGL;
    waveformGL = 0;
    delete waveformBeatsGL;
    waveformBeatsGL = 0;
    delete window;
    window = 0;
    delete m_waveformTexture;
    m_waveformTexture = 0;
    */
}

void Audio::quit()
{
    m_run = false;
}

void Audio::run() {
    PaError err = Pa_Initialize();
    PaStream *stream = NULL;

    if(err != paNoError) {
        qDebug() << "Could not initialize PortAudio";
        goto err;
    }

    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device < 0) {
        qWarning() << "Could not find input device, running without";
        while(m_run.load());
        goto err;
    }
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream,
                        &inputParameters,
                        0,
                        FrameRate,
                        ChunkSize,
                        paClipOff,
                        0,
                        0);
    if(err != paNoError) {
        qDebug() << "Could not open PortAudio input stream";
        goto err;
    }

    err = Pa_StartStream(stream);
    if(err != paNoError) {
        qDebug() << "Could not open audio input stream";
        goto err;
    }

    while(m_run.load()) {
        err = Pa_ReadStream(stream, &chunk[0], ChunkSize);
        if(err != paNoError) {
            qDebug() << "Could not read audio chunk";
            continue;
        }
        analyzeChunk();
        //qDebug() << "read chunk" << m_chunk;
    }

err:
    err = Pa_Terminate();
    if(err != paNoError) qDebug() << "Could not cleanly terminate PortAudio";
}

double Audio::hannWindow(int n) {
    return 0.5 * (1 - cos(2 * M_PI * n / (FFTLength - 1)));
}

void Audio::analyzeChunk() {
    // Add chunk samples to queue
    for(int i=0; i<ChunkSize; i++) {
        sampQueue[sampQueuePtr] = chunk[i];
        sampQueuePtr = (sampQueuePtr + 1) % FFTLength;
    }

    // Window the data in queue and prepare it for FFTW
    for(int i=0; i<FFTLength; i++) {
        fftIn[i] = sampQueue[(sampQueuePtr + i) % FFTLength] * window[i];
    }

    // Run the FFT
    fftwf_execute(m_plan);

    // Convert to spectrum (log(freq))
    std::fill(spectrum.begin(),spectrum.end(),0);
    std::fill(spectrumCount.begin(),spectrumCount.end(),0);
    auto binFactor = SpectrumBins / std::log(FFTLength / 2.0f);
    for(auto i=1; i<FFTLength / 2; i++) {
        auto bin = (int)(std::log(float(i)) * binFactor);
        spectrum[bin] += fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1];
        spectrumCount[bin]++;
    }

    // Convert to spectrum (log(power))
    for(auto i=0; i<SpectrumBins; i++) {
        if(spectrumCount[i] != 0) {
            spectrum[i] = SpectrumGain * (std::log1p(spectrum[i]) - SpectrumOffset);
            if(spectrum[i] < 0) spectrum[i] = 0;
            if(spectrum[i] > 1) spectrum[i] = 1;
        }
    }

    // Fill in holes
    for(auto i=0; i<SpectrumBins; i++) {
        if(spectrumCount[i] == 0) {
            auto before = i;
            auto after = i;
            for (; before >= 0; before--) {
                if (spectrumCount[before] != 0) break;
            }
            for (; after < SpectrumBins; after++) {
                if (spectrumCount[after] != 0) break;
            }
            float weightAfter = (float)(i - before) / (after - before);
            float weightBefore = (float)(after - i) / (after - before);
            weightAfter *= weightAfter;
            weightBefore *= weightBefore;
            spectrum[i] = (weightBefore * spectrum[before] + weightAfter * spectrum[after]) / (weightAfter + weightBefore);
        }
    }

    // Diode-LPF and tally up hi, mid, low
    double hi = 0;
    double mid = 0;
    double low = 0;
    double level = 0;

    for(auto i=0; i<SpectrumBins; i++) {
        if(spectrum[i] > spectrumLPF[i]) {
            spectrumLPF[i] = spectrum[i] * SpectrumUpAlpha + spectrumLPF[i] * (1 - SpectrumUpAlpha);
        } else {
            spectrumLPF[i] = spectrum[i] * SpectrumDownAlpha + spectrumLPF[i] * (1 - SpectrumDownAlpha);
        }
        auto freqFrac = (float)i / SpectrumBins;
        if(freqFrac < LowCutoff) {
            low += spectrumLPF[i];
        } else if(freqFrac > HiCutoff) {
            hi += spectrumLPF[i];
        } else {
            mid += spectrumLPF[i];
        }
    }

    // Pass to BTrack. TODO: use already FFT'd values
    btrack_process_audio_frame(&btrack, chunk.data());

    auto btrackBPM = btrack_get_bpm(&btrack);
    m_timebase->update(Timebase::TimeSourceAudio, Timebase::TimeSourceEventBPM, btrackBPM);
    auto msUntilBeat = btrack_get_time_until_beat(&btrack) * 1000.;
    m_timebase->update(Timebase::TimeSourceAudio, Timebase::TimeSourceEventBeat, msUntilBeat);

    if (btrack_beat_due_in_current_frame(&btrack)) {
        //INFO("Beat; BPM=%lf", btrack_get_bpm(&btrack));
        if (m_timebase->beatIndex() % 4 == 0)
            beatLPF = 1.0;
        else
            beatLPF = 0.6;
    } else {
        beatLPF *= 0.95;
    }

    {
        QMutexLocker locker(&m_audioLock);
        audioThreadHi = hi / (1. - HiCutoff) * WaveformGain;
        audioThreadMid = mid / (HiCutoff - LowCutoff) * WaveformGain;
        audioThreadLow = low / LowCutoff * WaveformGain;

        level = audioThreadHi;
        if(audioThreadMid > level) level = audioThreadMid;
        if(audioThreadLow > level) level = audioThreadLow;

        if(level > audioThreadLevel) {
            level = level * LevelUpAlpha + audioThreadLevel * (1 - LevelUpAlpha);
        } else {
            level = level * LevelDownAlpha + audioThreadLevel * (1 - LevelDownAlpha);
        }

        audioThreadLevel = level;

        std::copy(spectrumLPF.begin(),spectrumLPF.end(),spectrumGL.begin());
        waveformGL[waveformPtr * 4 + 0] = audioThreadHi;
        waveformGL[waveformPtr * 4 + 1] = audioThreadMid;
        waveformGL[waveformPtr * 4 + 2] = audioThreadLow;
        waveformGL[waveformPtr * 4 + 3] = audioThreadLevel;
        waveformBeatsGL[waveformPtr * 4] = beatLPF;

        {
            auto it = waveformGL.begin() + waveformPtr * 4;
            std::copy_n(it, 4, it + WaveformLength * 4);
        }
        {
            auto it = waveformBeatsGL.begin() + waveformPtr * 4;
            std::copy_n(it, 1, it + WaveformLength * 4);
        }
        waveformPtr = (waveformPtr + 1) % WaveformLength;
    }
}

// This is called from the OpenGL Thread
void Audio::levels(double *audioHi, double *audioMid, double *audioLow, double *audioLevel) {
    QMutexLocker locker(&m_audioLock);
    /*
    glBindTexture(GL_TEXTURE_1D, tex_spectrum);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, config.audio.spectrum_bins, 0, GL_RED, GL_FLOAT, spectrum_gl);
    glBindTexture(GL_TEXTURE_1D, tex_waveform);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, config.audio.waveform_length, 0, GL_RGBA, GL_FLOAT, &waveform_gl[waveform_ptr * 4]);
    glBindTexture(GL_TEXTURE_1D, tex_waveform_beats);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, config.audio.waveform_length, 0, GL_RGBA, GL_FLOAT, &waveform_beats_gl[waveform_ptr * 4]);
    glBindTexture(GL_TEXTURE_1D, 0);
    */

    *audioHi = audioThreadHi;
    *audioMid = audioThreadMid;
    *audioLow = audioThreadLow;
    *audioLevel = audioThreadLevel;
}

void Audio::renderGraphics() {
    QMutexLocker locker(&m_audioLock);
    if(m_waveformTexture == NULL || m_waveformTexture->width() != WaveformLength) {
        delete m_waveformTexture;
        m_waveformTexture = new QOpenGLTexture(QOpenGLTexture::Target1D);
        m_waveformTexture->setSize(WaveformLength);
        m_waveformTexture->setFormat(QOpenGLTexture::RGBA32F);
        m_waveformTexture->allocateStorage();
    }
    if(m_waveformBeatsTexture == NULL || m_waveformBeatsTexture->width() != WaveformLength) {
        delete m_waveformBeatsTexture;
        m_waveformBeatsTexture = new QOpenGLTexture(QOpenGLTexture::Target1D);
        m_waveformBeatsTexture->setSize(WaveformLength);
        m_waveformBeatsTexture->setFormat(QOpenGLTexture::RGBA32F);
        m_waveformBeatsTexture->allocateStorage();
    }
    if(m_spectrumTexture == NULL || m_spectrumTexture->width() != SpectrumBins) {
        delete m_spectrumTexture;
        m_spectrumTexture = new QOpenGLTexture(QOpenGLTexture::Target1D);
        m_spectrumTexture->setSize(SpectrumBins);
        m_spectrumTexture->setFormat(QOpenGLTexture::R32F);
        m_spectrumTexture->allocateStorage();
    }
    m_waveformTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, &waveformGL[waveformPtr * 4]);
    m_waveformBeatsTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, &waveformBeatsGL[waveformPtr * 4]);
    m_spectrumTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, spectrumGL.data());
}
