#pragma once

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QEvent>
#include <QThread>
#include "VideoNode.h"

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
    ~RenderContext();
    QOffscreenSurface *surface;
    QOpenGLContext *context;
    QTimer *timer;
    QElapsedTimer elapsed_timer;
    QMutex m_masterLock;
    QMutex m_contextLock;
    void moveToThread(QThread *t);

    void makeCurrent();
    void flush();
    void setMaster(VideoNode *e);
    void share(QOpenGLContext *current);
    VideoNode *master();

public slots:
    void start();
    void render();

private:
    VideoNode *m_master;

signals:
    void renderingFinished();
};
