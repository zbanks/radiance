#include "VideoNode.h"
#include "Model.h"
#include <QDebug>
#include <QtQml>

VideoNode::VideoNode(const VideoNode &other)
    : m_inputCount(other.m_inputCount)
    , m_id(other.m_id)
    , m_context(other.m_context)
    , m_workerContext(other.m_workerContext) {
}
QSharedPointer<OpenGLWorkerContext> VideoNode::workerContext() const {
    return m_workerContext;
}

VideoNode::~VideoNode() = default;

int VideoNode::inputCount() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_inputCount;
}

void VideoNode::setInputCount(int value) {
    Q_ASSERT(QThread::currentThread() == thread());

    if (value != m_inputCount) {
        {
            QMutexLocker locker(&m_stateLock);
            m_inputCount = value;
        }
        emit inputCountChanged(value);
    }
}

int VideoNode::id() {
    QMutexLocker locker(&m_idLock);
    return m_id;
}

void VideoNode::setId(int id) {
    {
        QMutexLocker locker(&m_idLock);
        m_id = id;
    }
    emit idChanged(id);
}

QList<QSharedPointer<Chain>> VideoNode::chains() {
    return m_chains;
}

void VideoNode::setChains(QList<QSharedPointer<Chain>> chains) {
    QList<QSharedPointer<Chain>> toRemove = m_chains;
    QList<QSharedPointer<Chain>> toAdd;
    for (int i=0; i<chains.count(); i++) {
        if (m_chains.contains(chains.at(i))) {
            // If it exists already, don't remove it
            toRemove.removeAll(chains.at(i));
        } else {
            // If it doesn't exist already, add it
            toAdd.append(chains.at(i)); // Add it
        }
    }
    chainsEdited(toAdd, toRemove);
    m_chains = chains;
    emit chainsChanged(chains);
}

Context *VideoNode::context() {
    return m_context;
}
