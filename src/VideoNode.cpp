#include "VideoNode.h"
#include "RenderContext.h"
#include "Model.h"

VideoNode::VideoNode(QSharedPointer<RenderContext> context)
    : m_context(context)
    , m_inputCount(0) 
    , m_ready(false)
{
}

VideoNode::VideoNode(const VideoNode &other)
    : m_context(other.m_context)
    , m_inputCount(other.m_inputCount)
    , m_ready(other.m_ready)
{
}

VideoNode::~VideoNode() {
}

int VideoNode::inputCount() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_inputCount;
}

bool VideoNode::ready() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_ready;
}

QSize VideoNode::size(int chain) {
    return m_context->chainSize(chain);
}

QSharedPointer<RenderContext> VideoNode::context() {
    return m_context;
}

void VideoNode::setInputCount(int value) {
    Q_ASSERT(QThread::currentThread() == thread());

    if(value != m_inputCount) {
        {
            QMutexLocker locker(&m_stateLock);
            m_inputCount = value;
        }
        emit inputCountChanged(value);
    }
}

void VideoNode::setReady(bool value) {
    Q_ASSERT(QThread::currentThread() == thread());

    if(value != m_ready) {
        {
            QMutexLocker locker(&m_stateLock);
            m_ready = value;
        }
        emit readyChanged(value);
    }
}
