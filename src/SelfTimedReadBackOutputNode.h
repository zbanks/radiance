#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include "OutputNode.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

class STRBONOpenGLWorker;
class SelfTimedReadBackOutputNodePrivate;

class SelfTimedReadBackOutputNode
    : public OutputNode {
    Q_OBJECT

    friend class WeakSelfTimedReadBackOutputNode;
    friend class STRBONOpenGLWorker;

public:
    SelfTimedReadBackOutputNode(Context *context, QSize chainSize, long msec=0);
    SelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other);
    SelfTimedReadBackOutputNode *clone() const override;

public slots:
    void start();
    void stop();
    void setInterval(long msec);
    void force();

signals:
    void initialize();
    void frame(QSize size, QByteArray frame);

private:
    SelfTimedReadBackOutputNode(QSharedPointer<SelfTimedReadBackOutputNodePrivate> other_ptr);
    QSharedPointer<SelfTimedReadBackOutputNodePrivate> d() const;
};

///////////////////////////////////////////////////////////////////////////////

class WeakSelfTimedReadBackOutputNode {
public:
    WeakSelfTimedReadBackOutputNode();
    WeakSelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other);
    QSharedPointer<SelfTimedReadBackOutputNodePrivate> toStrongRef();

protected:
    QWeakPointer<SelfTimedReadBackOutputNodePrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////

class STRBONOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    STRBONOpenGLWorker(SelfTimedReadBackOutputNode p);

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
    WeakSelfTimedReadBackOutputNode m_p;
    QTimer *m_timer{};
    QByteArray m_pixelBuffer;
    QSize m_size;
    QSharedPointer<QOpenGLShaderProgram> m_shader;
    QSharedPointer<QOpenGLFramebufferObject> m_fbo;
};

class SelfTimedReadBackOutputNodePrivate : public OutputNodePrivate {
public:
    SelfTimedReadBackOutputNodePrivate(Context *context, QSize chainSize);

    OpenGLWorkerContext *m_workerContext{};
    QSharedPointer<STRBONOpenGLWorker> m_worker;
};
