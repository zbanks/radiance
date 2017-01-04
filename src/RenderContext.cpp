#include "RenderContext.h"
#include <QOpenGLFunctions>
#include <QDebug>

RenderContext::RenderContext() : context(0), surface(0), m_master(0) {
    context = new QOpenGLContext();
    context->create();

    surface = new QOffscreenSurface();
    surface->setFormat(context->format());
    surface->create();

    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &RenderContext::render);
}

void RenderContext::moveToThread(QThread *t) {
    QObject::moveToThread(t);
    context->moveToThread(thread());
}

RenderContext::~RenderContext() {
    delete surface;
    surface = 0;
    delete context;
    context = 0;
}

void RenderContext::start() {
    timer->start();
}

// Radiance creates its own OpenGL context for rendering,
// in case it is running with no UI. If there is a Qt UI,
// you will need to call this function with the UI context
// to properly set up context sharing.
void RenderContext::share(QOpenGLContext *current) {
    // We re-create and then delete the old Context & Surface
    // so that the newly created once will have different
    // pointers. This way, anything using this context
    // can watch the context pointer for changes
    // to see if a context has been re-created
    // since the last render.

    m_contextLock.lock();
    qDebug() << "sharing is caring";
    context->doneCurrent();
    QOpenGLContext *newContext = new QOpenGLContext();
    delete context;
    context = newContext;
    QSurfaceFormat f = current->format();
    //f.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    context->setFormat(f);
    context->setShareContext(current);
    context->create();
    context->moveToThread(thread());
    QOffscreenSurface *newSurface = new QOffscreenSurface();
    delete surface;
    surface = newSurface;
    surface->setFormat(context->format());
    surface->create();
    qDebug() << "done caring";
    m_contextLock.unlock();
}

void RenderContext::setMaster(Effect *e) {
    m_masterLock.lock();
    m_master = e;
    m_masterLock.unlock();
}

Effect *RenderContext::master() {
    Effect *e;
    m_masterLock.lock();
    e = m_master;
    m_masterLock.unlock();
    return e;
}

void RenderContext::render() {
    qDebug() << "TICK" << QThread::currentThread();
    if(m_master != NULL) {
        m_contextLock.lock();
        m_master->render();
        m_contextLock.unlock();
    }
}

void RenderContext::makeCurrent() {
    context->makeCurrent(surface);
}

void RenderContext::flush() {
    //context->functions()->glFlush();
    context->functions()->glFinish();
    //context->swapBuffers(surface);
}
