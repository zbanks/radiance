#include "SelfTimedReadBackOutputNode.h"

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(Context *context, QSize chainSize)
    : OutputNode(context, chainSize) {
    m_workerContext = new OpenGLWorkerContext();
    m_workerContext->setParent(this);
    m_worker = new STRBONOpenGLWorker(this);

    qDebug() << "My thread:" << QThread::currentThread();
    qDebug() << "worker thread:" << m_worker->thread();

    bool result = QMetaObject::invokeMethod(m_worker, "initialize");
    Q_ASSERT(result);
}

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other)
    : OutputNode(other)
    , m_workerContext(other.m_workerContext) {
}

SelfTimedReadBackOutputNode::~SelfTimedReadBackOutputNode() {
}

STRBONOpenGLWorker::STRBONOpenGLWorker(SelfTimedReadBackOutputNode *p)
    : OpenGLWorker(p->m_workerContext)
    , m_p(p) {
}

STRBONOpenGLWorker::~STRBONOpenGLWorker() {
}

void STRBONOpenGLWorker::initialize() {
    m_timer = new QTimer(this);
    qDebug() << "Initialize called in:" << QThread::currentThread();
    connect(m_timer, &QTimer::timeout, this, &STRBONOpenGLWorker::onTimeout);
    m_timer->setInterval(1000);
    m_timer->start();
}

void STRBONOpenGLWorker::onTimeout() {
    qDebug() << "TICK";
    GLuint texture = m_p->render();
    glBindTexture(GL_TEXTURE_2D, texture);
    auto size = m_p->chain()->size();
    m_pixelBuffer.resize(3 * size.width() * size.height());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pixelBuffer.data());
    m_p->frame(size, &m_pixelBuffer);
}
