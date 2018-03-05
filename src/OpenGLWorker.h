#pragma once

#include <QObject>
#include <QOpenGLContext>

// OpenGLWorkers are used
// when an OpenGL context is required
// outside of a rendering operation
// (for instance, during initialization
// to load in textures.)
// OpenGLWorkers live in a different thread
// so as to not block rendering,
// and must be treated accordingly
// i.e. use queued signals
// and thread-safe mutator methods

class OpenGLWorkerContext;

class OpenGLWorker : public QObject {
    Q_OBJECT

public:
    OpenGLWorker(OpenGLWorkerContext *context);
   ~OpenGLWorker() override;

    // This method switches to the worker's
    // OpenGL context.
    // It must be called
    // in the correct thread
    // before using any OpenGL functions.
    void makeCurrent();

    // This method returns that context
    QOpenGLContext *openGLContext();

    // This method allows access to OpenGL functions
    // within the context
    QOpenGLFunctions *glFuncs();
private:
    OpenGLWorkerContext *m_context;
};
