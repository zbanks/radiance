#pragma once

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QEvent>
#include <QSet>
#include <QOpenGLShaderProgram>
#include "VideoNode.h"

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
   ~RenderContext() override;
    QOffscreenSurface *surface;
    QOpenGLContext *context;
    QTimer *timer;
    QElapsedTimer elapsed_timer;
    QMutex m_contextLock;

    void makeCurrent();
    void flush();

    QOpenGLShaderProgram *m_premultiply;

    QSet<VideoNode*> m_videoNodes; // temp
    int outputCount();
    int previewFboIndex();
    int outputFboIndex();
    QSize fboSize(int i);

public slots:
    void start();
    void render();
    void addVideoNode(VideoNode* n);
    void removeVideoNode(VideoNode* n);
    void addSyncSource(QObject *source);
    void removeSyncSource(QObject *source);

private:
    QList<VideoNode*> topoSort();
    void load();
    int m_outputCount;
    QList<QObject *> m_syncSources;
    QObject *m_currentSyncSource;

signals:
    void renderingFinished();
    void addVideoNodeRequested(VideoNode *n);
    void removeVideoNodeRequested(VideoNode *n);
};
