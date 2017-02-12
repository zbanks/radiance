#include "Audio.h"

#include <QDebug>
#include <portaudio.h>
#include <cmath>

const int FrameRate = 44100;
const int ChunkSize = 128;

Audio::Audio(QObject *p)
    : QThread(p)
    , m_chunk(ChunkSize)
    , m_run(true)
    , m_time(0)
{
    setObjectName("AudioThread");
    start();
}
Audio::~Audio()
{
    quit();
    wait();
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
