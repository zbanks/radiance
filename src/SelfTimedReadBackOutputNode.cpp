#include "SelfTimedReadBackOutputNode.h"

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(Context *context, QSize chainSize, long msec)
    : OutputNode(new SelfTimedReadBackOutputNodePrivate(context, chainSize)) {

    d()->m_workerContext = new OpenGLWorkerContext();
    d()->m_worker = QSharedPointer<STRBONOpenGLWorker>(new STRBONOpenGLWorker(*this), &QObject::deleteLater);
    connect(d()->m_worker.data(), &QObject::destroyed, d()->m_workerContext, &QObject::deleteLater);

    d()->m_chain.moveToWorkerContext(d()->m_workerContext);

    {
        auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "initialize");
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

QSharedPointer<SelfTimedReadBackOutputNodePrivate> SelfTimedReadBackOutputNode::d() const {
    return d_ptr.staticCast<SelfTimedReadBackOutputNodePrivate>();
}

SelfTimedReadBackOutputNode::SelfTimedReadBackOutputNode(QSharedPointer<SelfTimedReadBackOutputNodePrivate> other_ptr)
    : OutputNode(other_ptr.staticCast<OutputNodePrivate>())
{
}

void SelfTimedReadBackOutputNode::start() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "start");
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::stop() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "stop");
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::setInterval(long msec) {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "setInterval", Q_ARG(long, msec));
    Q_ASSERT(result);
}

void SelfTimedReadBackOutputNode::initialize() {
}

void SelfTimedReadBackOutputNode::frame(QSize size, QByteArray frame) {
}

// WeakMovieNode methods

WeakSelfTimedReadBackOutputNode::WeakSelfTimedReadBackOutputNode()
{
}

WeakSelfTimedReadBackOutputNode::WeakSelfTimedReadBackOutputNode(const SelfTimedReadBackOutputNode &other)
    : d_ptr(other.d())
{
}

QSharedPointer<SelfTimedReadBackOutputNodePrivate> WeakSelfTimedReadBackOutputNode::toStrongRef() {
    return d_ptr.toStrongRef();
}

// STRBONOpenGLWorker methods

STRBONOpenGLWorker::STRBONOpenGLWorker(SelfTimedReadBackOutputNode p)
    : OpenGLWorker(p.d()->m_workerContext)
    , m_p(p) {
}

void STRBONOpenGLWorker::initialize() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &STRBONOpenGLWorker::onTimeout);
}

void STRBONOpenGLWorker::setInterval(long msec) {
    Q_ASSERT(m_timer != nullptr);
    m_timer->setInterval(msec);
}

void STRBONOpenGLWorker::start() {
    Q_ASSERT(m_timer != nullptr);
    m_timer->start();
}

void STRBONOpenGLWorker::stop() {
    Q_ASSERT(m_timer != nullptr);
    m_timer->start();
}

void STRBONOpenGLWorker::onTimeout() {
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // SelfTimedReadBackOutputNode was deleted
    SelfTimedReadBackOutputNode p(d);

    //qDebug() << "Render...";
    makeCurrent();
    GLuint texture = p.render();
    auto size = p.chain().size();

    m_pixelBuffer.resize(3 * size.width() * size.height());
    //qDebug() << "Texture ID is:" << texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pixelBuffer.data());
    p.frame(size, m_pixelBuffer);
    //qDebug() << "Rendered.";
}

SelfTimedReadBackOutputNodePrivate::SelfTimedReadBackOutputNodePrivate(Context *context, QSize chainSize)
    : OutputNodePrivate(context, chainSize)
{
}
