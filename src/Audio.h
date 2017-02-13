#pragma once

#include <QObject>
#include <QThread>
#include <QVector>
#include <QAtomicInt>
#include <QMutex>
#include <fftw3.h>
#include <QOpenGLFunctions>
#include "BTrack.h"

class Audio : public QThread {
    Q_OBJECT

public:
    Audio(QObject *p = nullptr);
   ~Audio() override;
    double time();

protected:
    void run();
    QMutex m_audioLock;

private:
    QVector<float> m_chunk;
    QAtomicInt m_run;
    double m_time;

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
    struct btrack btrack; 
    double *window;

    static double hannWindow(int n);

public slots:
    void quit();
};
