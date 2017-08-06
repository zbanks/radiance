#include "OpenGLWorker.h"
#include "main.h"

OpenGLWorker::OpenGLWorker(OpenGLWorkerContext *context) {
    m_context = context;
    moveToThread(context);
}

OpenGLWorker::~OpenGLWorker() {
}

void OpenGLWorker::makeCurrent() {
    m_context->makeCurrent();
}
