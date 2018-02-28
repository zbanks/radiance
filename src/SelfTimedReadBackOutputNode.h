#pragma once

#include "OutputNode.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

class STRBONOpenGLWorker;

class SelfTimedReadBackOutputNode
    : public OutputNode {
    Q_OBJECT

    friend class STRBONOpenGLWorker;

public:
    SelfTimedReadBackOutputNode(Context *context, QSize chainSize);
    SelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other);
    ~SelfTimedReadBackOutputNode();

public slots:
    virtual void frame(QSize size, QByteArray *frame) = 0;

signals:

protected:

protected slots:

private:
    OpenGLWorkerContext *m_workerContext{};
    STRBONOpenGLWorker *m_worker{};
};

class STRBONOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    STRBONOpenGLWorker(SelfTimedReadBackOutputNode *p);
   ~STRBONOpenGLWorker();
public slots:
    void initialize();
signals:
    // This is emitted when it is done
    void initialized();
protected slots:
    void onTimeout();
protected:
    SelfTimedReadBackOutputNode *m_p{};
    QTimer *m_timer{};
    QByteArray m_pixelBuffer;
};

