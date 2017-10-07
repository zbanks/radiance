#pragma once

#include <QObject>
#include <QThread>
#include <QVector>
#include <QAtomicInt>
#include <QMutex>
#include <QtMultimedia/QAudioInput>
#include <fftw3.h>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include "BTrack.h"

class Audio : public QThread {
    Q_OBJECT

public:
    Audio(QObject *p = nullptr);
   ~Audio() override;
    double time();
    void levels(double *audioHi, double *audioMid, double *audioLow, double *audioLevel);
    void renderGraphics();
    QOpenGLTexture *m_waveformTexture;
    QOpenGLTexture *m_waveformBeatsTexture;
    QOpenGLTexture *m_spectrumTexture;

protected:
    void run();
    QMutex m_audioLock;

private:
    QAtomicInt m_run;
    double m_time;

    QAudioInput *m_audio;

    float *chunk;
    float *sampQueue;
    int sampQueuePtr;
    fftw_plan plan;
    double *fftIn;
    fftw_complex *fftOut;
    double *spectrum;
    double *spectrumLPF;
    GLfloat *spectrumGL;
    int *spectrumCount;
    GLfloat *waveformGL;
    GLfloat *waveformBeatsGL;
    int waveformPtr;
    double audioThreadHi;
    double audioThreadMid;
    double audioThreadLow;
    double audioThreadLevel;
    double beatLPF;
    struct btrack btrack; 
    double *window;

    static double hannWindow(int n);
    void analyzeChunk();

public slots:
    void quit();
};
