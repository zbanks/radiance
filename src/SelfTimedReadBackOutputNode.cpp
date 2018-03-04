#include "SelfTimedReadBackOutputNode.h"

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(Context *context, QSize chainSize, long msec)
    : OutputNode(new SelfTimedReadBackOutputNodePrivate(context, chainSize)) {

    d()->m_workerContext = new OpenGLWorkerContext();
    d()->m_workerContext->setParent(this);
    d()->m_worker = new STRBONOpenGLWorker(this);

    {
        auto result = QMetaObject::invokeMethod(d()->m_worker, "initialize");
        Q_ASSERT(result);
    }
    if (msec != 0) {
        setInterval(msec);
    }
}

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other)
    : OutputNode(other)
{
}

SelfTimedReadBackOutputNode *SelfTimedReadBackOutputNode::clone() const {
    return new SelfTimedReadBackOutputNode(*this);
}

QSharedPointer<SelfTimedReadBackOutputNodePrivate> SelfTimedReadBackOutputNode::d() {
    return d_ptr.staticCast<SelfTimedReadBackOutputNodePrivate>();
}

void SelfTimedReadBackOutputNode::start() {
    auto result = QMetaObject::invokeMethod(d()->m_worker, "start");
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::stop() {
    auto result = QMetaObject::invokeMethod(d()->m_worker, "stop");
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::setInterval(long msec) {
    auto result = QMetaObject::invokeMethod(d()->m_worker, "setInterval", Q_ARG(long, msec));
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::initialize() {
}

void SelfTimedReadBackOutputNode::frame(QSize size, QByteArray frame) {
}

STRBONOpenGLWorker::STRBONOpenGLWorker(SelfTimedReadBackOutputNode *p)
    : OpenGLWorker(p->d()->m_workerContext)
    , m_p(p) {
}

STRBONOpenGLWorker::~STRBONOpenGLWorker() {
}

void STRBONOpenGLWorker::initialize() {
    Q_ASSERT(QThread::currentThread() == thread());
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &STRBONOpenGLWorker::onTimeout);
    m_p->initialize();
}

void STRBONOpenGLWorker::setInterval(long msec) {
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(m_timer != nullptr);
    m_timer->setInterval(msec);
}

void STRBONOpenGLWorker::start() {
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(m_timer != nullptr);
    m_timer->start();
}

void STRBONOpenGLWorker::stop() {
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(m_timer != nullptr);
    m_timer->start();
}

void STRBONOpenGLWorker::onTimeout() {
    Q_ASSERT(QThread::currentThread() == thread());

    qDebug() << "Render...";
    GLuint texture = m_p->render();
    auto size = m_p->chain().size();
    m_pixelBuffer.resize(3 * size.width() * size.height());
    qDebug() << "Texture ID is:" << texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pixelBuffer.data());
    m_p->frame(size, m_pixelBuffer);
    qDebug() << "Rendered.";
}

SelfTimedReadBackOutputNodePrivate::SelfTimedReadBackOutputNodePrivate(Context *context, QSize chainSize)
    : OutputNodePrivate(context, chainSize)
{
}
