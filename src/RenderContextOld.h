#pragma once

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QEvent>
#include <QSet>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QSemaphore>
#include "VideoNodeOld.h"
#include "semaphore.hpp"

class RenderContextOld : public QObject {
    Q_OBJECT

public:
    RenderContextOld();
   ~RenderContextOld() override;
    static QString defaultVertexShaderSource();
    QOffscreenSurface *surface;
    QOpenGLContext *context;
    QTimer *timer;
    QElapsedTimer elapsed_timer;
    QMutex m_contextLock;

    void makeCurrent();
    void flush();

    QOpenGLShaderProgram *m_premultiply;

    QSet<VideoNodeOld*> m_videoNodes; // temp
    int outputCount();
    int previewFboIndex();
    int outputFboIndex();
    QSize fboSize(int i);
    QOpenGLTexture *noiseTexture(int i);
    std::shared_ptr<QOpenGLFramebufferObject> &blankFbo();

public slots:
    void start();
    void update();
    void addVideoNodeOld(VideoNodeOld* n);
    void removeVideoNodeOld(VideoNodeOld* n);
    void addSyncSource(QObject *source);
    void removeSyncSource(QObject *source);
    qreal fps();

private slots:
    void render();

private:
    const qreal FPS_ALPHA = 0.03;
    QList<VideoNodeOld*> topoSort();
    void load();
    int m_outputCount;
    QList<QObject *> m_syncSources;
    QObject *m_currentSyncSource;
    radiance::RSemaphore m_rendering;
    QVector<QOpenGLTexture *> m_noiseTextures;
    void checkLoadShaders();
    void checkCreateNoise();
    void checkCreateBlankFbo();
    std::shared_ptr<QOpenGLFramebufferObject> m_blankFbo;
    qreal m_framePeriodLPF;

signals:
    void renderingFinished();
    void addVideoNodeOldRequested(VideoNodeOld *n);
    void removeVideoNodeOldRequested(VideoNodeOld *n);
    void renderRequested();
    void fpsChanged(qreal value);
};
