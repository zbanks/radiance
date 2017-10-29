#pragma once

#include <QObject>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <atomic>
#include <vector>
#include <memory>
#include <fftw3.h>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include "BTrack.h"
struct fftwf_mem_dtor {
    void operator () ( void *ptr) const {
        fftwf_free(ptr);
    };
};
class Audio : public QThread {
    Q_OBJECT

public:
    using size_type = std::vector<float>::size_type;
    using difference_type = std::vector<float>::difference_type;
    Audio(QObject *p = nullptr);
   ~Audio() override;
    double time();
    void levels(double *audioHi, double *audioMid, double *audioLow, double *audioLevel);
    void renderGraphics();
    QOpenGLTexture *m_waveformTexture{};
    QOpenGLTexture *m_waveformBeatsTexture{};
    QOpenGLTexture *m_spectrumTexture{};

protected:
    void run() override;
    QMutex m_audioLock;

private:
    std::atomic<bool> m_run{true};
    double m_time{};

    size_type m_size{2048};
    size_type m_coef{m_size ? (m_size/2 + 1) : 0};
    size_type m_bins{100};
    size_type m_len{512};
    std::vector<float> chunk = std::vector<float>(m_size, 0.f);
    std::vector<float> window = std::vector<float>(m_size, 0.f);
    std::vector<float> sampQueue = std::vector<float>(m_size, 0.f);
    int sampQueuePtr{};
    std::unique_ptr<float[],fftwf_mem_dtor> fftIn{fftwf_alloc_real(m_size)};
    std::unique_ptr<fftwf_complex[],fftwf_mem_dtor> fftOut{fftwf_alloc_complex(m_coef)};

    fftwf_plan m_plan = fftwf_plan_dft_r2c_1d(m_size,fftIn.get(),fftOut.get(),FFTW_ESTIMATE);
    std::vector<float> spectrum = std::vector<float>(m_bins, 0.f);
    std::vector<float> spectrumLPF = std::vector<float>(m_bins, 0.f);
    std::vector<GLfloat> spectrumGL = std::vector<GLfloat>(m_bins, 0.f);
    std::vector<int> spectrumCount = std::vector<int>(m_bins, 0);
    std::vector<GLfloat> waveformGL = std::vector<GLfloat>(m_len * 8, 0.f);
    std::vector<GLfloat> waveformBeatsGL = std::vector<GLfloat>(m_len * 8, 0.f);
    int waveformPtr{};
    double audioThreadHi{};
    double audioThreadMid{};
    double audioThreadLow{};
    double audioThreadLevel{};
    double beatLPF{};
    struct btrack btrack;

    static double hannWindow(int n);
    void analyzeChunk();

public slots:
    void quit();
};
