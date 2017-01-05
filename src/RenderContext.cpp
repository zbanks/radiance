#include "RenderContext.h"
#include <QOpenGLFunctions>
#include <QDebug>

RenderContext::RenderContext()
    : context(0)
    , surface(0)
    , m_master(0)
    , timer(0) {
    context = new QOpenGLContext();
    context->create();

    surface = new QOffscreenSurface();
    surface->setFormat(context->format());
    surface->create();
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
    elapsed_timer.start();
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
    m_contextLock.unlock();
}

void RenderContext::setMaster(VideoNode *e) {
    m_masterLock.lock();
    m_master = e;
    m_masterLock.unlock();
}

VideoNode *RenderContext::master() {
    VideoNode *e;
    m_masterLock.lock();
    e = m_master;
    m_masterLock.unlock();
    return e;
}

void RenderContext::render() {
    qint64 framePeriod = elapsed_timer.nsecsElapsed();
    elapsed_timer.restart();

    if(m_master != NULL) {
        m_contextLock.lock();
        m_master->render();
        m_contextLock.unlock();
    }
    emit renderingFinished();
    qint64 renderingPeriod = elapsed_timer.nsecsElapsed();
    //qDebug() << framePeriod << renderingPeriod;
}

void RenderContext::makeCurrent() {
    context->makeCurrent(surface);
}

void RenderContext::flush() {
    //context->functions()->glFlush();
    context->functions()->glFinish();
    //context->swapBuffers(surface);
}
