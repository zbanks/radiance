#include "RenderThread.h"
#include <QOpenGLFunctions>

RenderThread::RenderThread() : context(0), surface(0) {
    start();
}

void RenderThread::makeContext(QOpenGLContext *current) {
    context = new QOpenGLContext();
    context->setFormat(current->format());
    context->setShareContext(current);
    context->create();

    surface = new QOffscreenSurface();
    surface->setFormat(context->format());
    surface->create();

    context->moveToThread(this);
}

void RenderThread::makeCurrent() {
    context->makeCurrent(surface);
}

void RenderThread::flush() {
    context->functions()->glFlush();
}

void RenderThread::deleteContext() {
    context->doneCurrent();
    delete context;
    context = 0;
    delete surface;
    surface = 0;
}
