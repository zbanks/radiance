#pragma once

#include "OutputNode.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

class STRBONOpenGLWorker;
class SelfTimedReadBackOutputNodePrivate;

class SelfTimedReadBackOutputNode
    : public OutputNode {
    Q_OBJECT

    friend class STRBONOpenGLWorker;

public:
    SelfTimedReadBackOutputNode(Context *context, QSize chainSize, long msec=0);
    SelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other);
    SelfTimedReadBackOutputNode *clone() const override;

public slots:
    void start();
    void stop();
    void setInterval(long msec);

    // Reimplement these!
    virtual void frame(QSize size, QByteArray frame);
    virtual void initialize();

private:
    QSharedPointer<SelfTimedReadBackOutputNodePrivate> d() const;
};

class STRBONOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    STRBONOpenGLWorker(SelfTimedReadBackOutputNode *p);
   ~STRBONOpenGLWorker();
public slots:
    void initialize();
    void start();
    void stop();
    void setInterval(long msec);
protected slots:
    void onTimeout();
private:
    SelfTimedReadBackOutputNode *m_p{};
    QTimer *m_timer{};
    QByteArray m_pixelBuffer;
};

class SelfTimedReadBackOutputNodePrivate : public OutputNodePrivate {
public:
    SelfTimedReadBackOutputNodePrivate(Context *context, QSize chainSize);

    OpenGLWorkerContext *m_workerContext{};
    STRBONOpenGLWorker *m_worker{};
};
