#include "OpenGLWorkerContext.h"
#include <QOffscreenSurface>
#include <QDebug>

OpenGLWorkerContext::OpenGLWorkerContext(QSharedPointer<QSurface> surface)
    : m_surface(surface) {
    moveToThread(this); // I know, I know...
    connect(this, &QThread::started, this, &OpenGLWorkerContext::initialize);
}

OpenGLWorkerContext::~OpenGLWorkerContext() {
}

void OpenGLWorkerContext::initialize() {
    m_context = QSharedPointer<QOpenGLContext>(new QOpenGLContext());
    auto scontext = QOpenGLContext::globalShareContext();
    if(scontext) {
        m_context->setFormat(scontext->format());
        m_context->setShareContext(scontext);
    }

    m_context->create();

    if(m_surface.isNull()) {
        // Creating a QOffscreenSurface with no window
        // may fail on some platforms
        // (e.g. wayland)
        // On these platforms, you must pass in
        // a valid surface.
        QSharedPointer<QOffscreenSurface> surface(new QOffscreenSurface());
        surface->setFormat(m_context->format());
        surface->create();
        m_surface = surface;
    }
    makeCurrent();
    m_glFuncs = QSharedPointer<QOpenGLFunctions>(new QOpenGLFunctions(m_context.data()));
}

void OpenGLWorkerContext::makeCurrent() {
    //qDebug() << "Make context current" << this;
    m_context->makeCurrent(m_surface.data());
}

QOpenGLContext *OpenGLWorkerContext::context() {
    return m_context.data();
}

QOpenGLFunctions *OpenGLWorkerContext::glFuncs() {
    return m_glFuncs.data();
}
