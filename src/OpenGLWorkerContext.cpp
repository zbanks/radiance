#include "OpenGLWorkerContext.h"
#include <QOffscreenSurface>
#include <QDebug>

OpenGLWorkerContext::OpenGLWorkerContext(bool threaded, QSurface *surface)
    : m_surface(surface)
    , m_threaded(threaded) {
    if (m_threaded) {
        QThread *thread = new QThread();
        moveToThread(thread);
        connect(thread, &QThread::started, this, &OpenGLWorkerContext::initialize);
    } else {
        initialize();
    }
}

OpenGLWorkerContext::~OpenGLWorkerContext() {
    if (m_threaded) {
        thread()->quit();
        thread()->wait();
        delete thread();
    }
    m_context->doneCurrent();
    delete m_context;
}

void OpenGLWorkerContext::initialize() {
    m_context = new QOpenGLContext();
    auto scontext = QOpenGLContext::globalShareContext();
    if(scontext) {
        m_context->setFormat(scontext->format());
        m_context->setShareContext(scontext);
    }

    m_context->create();

    if(m_surface == nullptr) {
        // Creating a QOffscreenSurface with no window
        // may fail on some platforms
        // (e.g. wayland)
        // On these platforms, you must pass in
        // a valid surface.
        auto surface = new QOffscreenSurface();
        surface->setFormat(m_context->format());
        surface->create();
        surface->setParent(m_context);
        m_surface = surface;
    }
    makeCurrent();
}

void OpenGLWorkerContext::makeCurrent() {
    //qDebug() << "Make context current" << this;
    m_context->makeCurrent(m_surface);
}

QOpenGLContext *OpenGLWorkerContext::context() {
    return m_context;
}

QOpenGLFunctions *OpenGLWorkerContext::glFuncs() {
    return m_context->functions();
}
