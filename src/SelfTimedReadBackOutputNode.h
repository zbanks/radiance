#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include "OutputNode.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

// This class is a base class for
// writing your own simple outputs.

// You should subclass this base class if:
// 1. You just want the pixels in RAM, not on the GPU
//    (you don't want / need OpenGL features)
// 2. You want to process these frames in a new thread,
//    separate from Qt's render / GUI thread

// If so, this class is for you!

// Subclassing this is simple:
// 1. Connect to the "frame(QSize, QByteArray)" signal
//    with a direct connection to receive frames
// 2. Call start() / stop() / force() to get frames
// 3. Implement the static functions necessary to register your
//    output node in the registry (see registry.cpp)

// A new thread will be created,
// and frame() will be emitted in the context of this new thread.
// initialize() is also emitted in the context of the new thread,
// but just once at startup.

// For an example of how to use this class, check out
// ConsoleOutputNode

class STRBONOpenGLWorker;

class SelfTimedReadBackOutputNode
    : public OutputNode {
    Q_OBJECT

    friend class STRBONOpenGLWorker;

public:
    SelfTimedReadBackOutputNode(Context *context, QSize chainSize, long msec=0);

public slots:
    void start();
    void stop();
    void setInterval(long msec);
    void force();

signals:
    void initialize();
    void frame(QSize size, QByteArray frame);

protected:
    OpenGLWorkerContext *m_workerContext{};
    QSharedPointer<STRBONOpenGLWorker> m_worker;
};

typedef QmlSharedPointer<SelfTimedReadBackOutputNode, OutputNodeSP> SelfTimedReadBackOutputNodeSP;
Q_DECLARE_METATYPE(SelfTimedReadBackOutputNodeSP*)

///////////////////////////////////////////////////////////////////////////////

class STRBONOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    STRBONOpenGLWorker(SelfTimedReadBackOutputNodeSP p);

public slots:
    void initialize(QSize size);
    void start();
    void stop();
    void setInterval(long msec);
    void onTimeout();

signals:
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
    void initialized();
    void frame(QSize size, QByteArray pixelBuffer);

protected:
    QSharedPointer<QOpenGLShaderProgram> loadBlitShader();

private:
    QWeakPointer<SelfTimedReadBackOutputNode> m_p;
    QTimer *m_timer{};
    QByteArray m_pixelBuffer;
    QSize m_size;
    QSharedPointer<QOpenGLShaderProgram> m_shader;
    QSharedPointer<QOpenGLFramebufferObject> m_fbo;
};
