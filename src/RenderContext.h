#pragma once

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QEvent>
#include <QThread>
#include "Effect.h"

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
    void setMaster(Effect *e);
    void share(QOpenGLContext *current);
    Effect *master();

public slots:
    void start();
    void render();

private:
    Effect *m_master;

signals:
    void renderingFinished();
};
