#include "OpenGLWorkerContext.h"
#include <QOffscreenSurface>
#include <QDebug>

OpenGLWorkerContext::OpenGLWorkerContext(bool threaded, QSurface *surface)
    : m_surface(surface)
    , m_thread(nullptr) {
    if (threaded) {
        m_thread = new QThread();
        connect(m_thread, &QThread::started, this, &OpenGLWorkerContext::initialize, Qt::DirectConnection);
        connect(m_thread, &QThread::finished, this, &OpenGLWorkerContext::deinitialize, Qt::DirectConnection);
        //connect(this, &QObject::destroyed, this, &OpenGLWorkerContext::onDestroyed);
        m_thread->start();
    } else {
        initialize();
    }
}

void OpenGLWorkerContext::onDestroyed() {
    if (m_thread != nullptr) {
        m_thread->quit();
        m_thread->wait();
    } else {
        deinitialize();
    }
}

OpenGLWorkerContext::~OpenGLWorkerContext() {
    onDestroyed();
    delete m_thread;
    m_thread = nullptr;
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

void OpenGLWorkerContext::deinitialize() {
    if (m_context) {
        m_context->doneCurrent();
        delete m_context;
        m_context = nullptr;
    }
}

void OpenGLWorkerContext::makeCurrent() {
    //qDebug() << "Make context current" << this;
    m_context->makeCurrent(m_surface);
}

void OpenGLWorkerContext::moveToThread(QObject *obj) {
    if (m_thread != nullptr) {
        obj->moveToThread(m_thread);
    }
}

QOpenGLContext *OpenGLWorkerContext::context() {
    return m_context;
}

QOpenGLFunctions *OpenGLWorkerContext::glFuncs() {
    return m_context->functions();
}
