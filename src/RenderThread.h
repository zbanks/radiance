#ifndef __RENDER_THREAD_H
#define __RENDER_THREAD_H

#include <QThread>
#include <QOffscreenSurface>
#include <QOpenGLContext>

class RenderThread : public QThread {
public:
    RenderThread();
    QOffscreenSurface *surface;
    QOpenGLContext *context;

    void makeContext(QOpenGLContext *current);
    void makeCurrent();
    void flush();
    void deleteContext();
};

#endif
