#include "Audio.h"

#include <QDebug>
#include <portaudio.h>
#include <cmath>

const int FrameRate = 44100;
const int ChunkSize = 512;
const int FFTLength = 2048;
const int SpectrumBins = 100;
const int WaveformLength = 512;

Audio::Audio(QObject *p)
    : QThread(p)
    , m_chunk(ChunkSize)
    , m_run(true)
    , m_time(0)
    , sampQueue(0)
    , fftIn(0)
    , fftOut(0)
    , spectrum(0)
    , spectrumLPF(0)
    , spectrumGL(0)
    , waveformGL(0)
    , waveformBeatsGL(0)
    , window(0)
    , sampQueuePtr(0)
    , waveformPtr(0)
    , plan(0)
    , audioThreadHi(0)
    , audioThreadMid(0)
    , audioThreadLow(0)
    , audioThreadLevel(0)
{
    setObjectName("AudioThread");

    // Audio processing
    sampQueue = new float[FFTLength]();
    fftIn = new double[FFTLength]();
    fftOut = new fftw_complex[FFTLength / 2 + 1]();
    spectrum = new double[SpectrumBins]();
    spectrumLPF = new double[SpectrumBins]();
    spectrumGL = new GLfloat[SpectrumBins]();
    spectrumCount = new int[SpectrumBins]();
    waveformGL = new GLfloat[WaveformLength * 8]();
    waveformBeatsGL = new GLfloat[WaveformLength * 8]();
    window = new double[FFTLength];
    for(int i=0; i<FFTLength; i++) window[i] = hannWindow(i);

    plan = fftw_plan_dft_r2c_1d(FFTLength, fftIn, fftOut, FFTW_ESTIMATE);
    //if (btrack_init(&btrack, chunk_size, FFTLength, sample_rate) != 0)
    //if(btrack_init(&btrack, ChunkSize, 1024, FrameRate) != 0) throw std::runtime_error("Could not initialize BTrack");
    //if (time_master_register_source(&analyze_audio_time_source) != 0)
    //    PFAIL("Could not register btrack time source");

    start();
}

Audio::~Audio()
{
    quit();
    wait();
    fftw_destroy_plan(plan);
    btrack_del(&btrack);
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

    while(m_run) {
        err = Pa_ReadStream(stream, m_chunk.data(), ChunkSize);
        if(err != paNoError) {
            qDebug() << "Could not read audio chunk";
            goto err;
        }
        {
            QMutexLocker locker(&m_audioLock);
            m_time = fmod((m_time + 0.003), 128.);
        }
        //qDebug() << "read chunk" << m_chunk;
    }

err:
    err = Pa_Terminate();
    if(err != paNoError) qDebug() << "Could not cleanly terminate PortAudio";
}

double Audio::time() {
    QMutexLocker locker(&m_audioLock);
    return m_time;
}

double Audio::hannWindow(int n) {
    return 0.5 * (1 - cos(2 * M_PI * n / (FFTLength - 1)));
}
