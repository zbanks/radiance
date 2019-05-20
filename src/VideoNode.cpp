#include "VideoNode.h"
#include "Model.h"
#include <QtQml>

int VideoNode::inputCount() {
    QMutexLocker locker(&m_stateLock);
    return m_inputCount;
}

void VideoNode::setInputCount(int value) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if (value != m_inputCount) { 
            m_inputCount = value;
            changed = true;
        }
    }
    if (changed) emit inputCountChanged(value);
}

VideoNode::NodeState VideoNode::nodeState() {
    QMutexLocker locker(&m_stateLock);
    return m_nodeState;
}

void VideoNode::setNodeState(NodeState value) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if (value != m_nodeState) { 
            m_nodeState = value;
            changed = true;
        }
    }
    if (changed) emit nodeStateChanged(value);
}

QList<ChainSP> VideoNode::chains() {
    QMutexLocker locker(&m_stateLock);
    return m_chains;
}

void VideoNode::setChains(QList<ChainSP> chains) {
    bool wereChainsChanged = false;
    QList<ChainSP> toRemove;
    QList<ChainSP> toAdd;
    {
        QMutexLocker locker(&m_stateLock);
        toRemove = m_chains;
        for (int i=0; i<chains.count(); i++) {
            if (m_chains.contains(chains.at(i))) {
                // If it exists already, don't remove it
                toRemove.removeAll(chains.at(i));
            } else {
                // If it doesn't exist already, add it
                toAdd.append(chains.at(i)); // Add it
            }
        }
        if (!toAdd.empty() || !toRemove.empty()) {
            m_chains = chains;
            wereChainsChanged = true;
        }
    }
    if (wereChainsChanged) {
        chainsEdited(toAdd, toRemove);
        emit chainsChanged(chains);
    }
}

Context *VideoNode::context() {
    // Not mutable, so no need to lock
    return m_context;
}

QJsonObject VideoNode::serialize() {
    QJsonObject o;
    o.insert("type", metaObject()->className());
    return o;
}

QList<ChainSP> VideoNode::requestedChains() {
    return QList<ChainSP>();
}

void VideoNode::chainsEdited(QList<ChainSP> added, QList<ChainSP> removed) {
}

GLuint VideoNode::paint(ChainSP chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    Q_UNUSED(inputTextures);
    return 0;
}

void VideoNode::setLastModel(QWeakPointer<Model> model) {
    QMutexLocker locker(&m_stateLock);
    m_lastModel = model;
}

QWeakPointer<Model> VideoNode::lastModel() {
    QMutexLocker locker(&m_stateLock);
    return m_lastModel;
}

VideoNodePrivate::VideoNodePrivate(Context *context)
    : m_context(context)
{
}
