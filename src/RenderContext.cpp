#include "RenderContext.h"
#include <QOpenGLFunctions>
#include <QDebug>
#include <QThread>

RenderContext::RenderContext() : context(0), surface(0) {
}

void RenderContext::start() {
    context = new QOpenGLContext();
    context->create();

    surface = new QOffscreenSurface();
    surface->setFormat(context->format());
    surface->create();

    timer = new QTimer();
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &RenderContext::tick);
    timer->start();
}

void RenderContext::finish() {
    context->doneCurrent();
    delete context;
    context = 0;
    delete surface;
    surface = 0;
    delete timer;
    timer = 0;
}

// Radiance creates its own OpenGL context for rendering,
// in case it is running with no UI. If there is a Qt UI,
// you will need to call this function with the UI context
// to properly set up context sharing.
void RenderContext::share(QOpenGLContext *current) {
    context->setFormat(current->format());
    context->setShareContext(current);
    context->create();
    surface->setFormat(context->format());
    surface->create();
}

void RenderContext::tick() {
    qDebug() << "TICK";
    qDebug() << QThread::currentThread();
}

void RenderContext::makeCurrent() {
    context->makeCurrent(surface);
}

void RenderContext::flush() {
    context->functions()->glFlush();
}
