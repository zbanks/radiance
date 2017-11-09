#include "OpenGLWorker.h"
#include "main.h"

OpenGLWorker::OpenGLWorker(OpenGLWorkerContext *context)
    : OpenGLWorker(context->sharedFromThis())
{ }
OpenGLWorker::OpenGLWorker(QSharedPointer<OpenGLWorkerContext> context)
    : m_context(context)
{
    m_context->takeObject(this);
}

OpenGLWorker::~OpenGLWorker() {
}

void OpenGLWorker::makeCurrent() {
    m_context->makeCurrent();
}

QOpenGLContext *OpenGLWorker::openGLContext() {
    return m_context->context();
}

QOpenGLFunctions *OpenGLWorker::glFuncs() {
    return m_context->glFuncs();
}
