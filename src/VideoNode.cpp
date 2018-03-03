#include "VideoNode.h"
#include "Model.h"
#include <QDebug>
#include <QtQml>

VideoNode::VideoNode(Context *context)
    : m_context(context) {
}

VideoNode::VideoNode(const VideoNode &other)
    : m_inputCount(other.m_inputCount)
    , m_id(other.m_id)
    , m_context(other.m_context)
    , m_copy(true) {
}

VideoNode::~VideoNode() = default;

int VideoNode::inputCount() {
    Q_ASSERT(QThread::currentThread() == thread());
    QMutexLocker locker(&m_stateLock);
    return m_inputCount;
}

void VideoNode::setInputCount(int value) {
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(!m_copy);

    if (value != m_inputCount) {
        {
            QMutexLocker locker(&m_stateLock);
            m_inputCount = value;
        }
        emit inputCountChanged(value);
    }
}

int VideoNode::id() {
    QMutexLocker locker(&m_stateLock);
    return m_id;
}

void VideoNode::setId(int id) {
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(!m_copy);
    {
        QMutexLocker locker(&m_stateLock);
        m_id = id;
    }
    emit idChanged(id);
}

QList<Chain> VideoNode::chains() {
    QMutexLocker locker(&m_stateLock);
    return m_chains;
}

void VideoNode::setChains(QList<Chain> chains) {
    QList<Chain> toRemove = m_chains;
    QList<Chain> toAdd;
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

QJsonObject VideoNode::serialize() {
    QJsonObject o;
    o.insert("type", metaObject()->className());
    return o;
}

QList<Chain> VideoNode::requestedChains() {
    return QList<Chain>();
}

void VideoNode::chainsEdited(QList<Chain> added, QList<Chain> removed) {
}
