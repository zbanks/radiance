#include "VideoNode.h"
#include "Model.h"
#include <QtQml>

VideoNode::VideoNode(Context *context)
    : m_context(context)
{
}

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

QList<QSharedPointer<Chain>> VideoNode::chains() {
    QMutexLocker locker(&m_stateLock);
    return m_chains;
}

void VideoNode::setChains(QList<QSharedPointer<Chain>> chains) {
    bool wereChainsChanged = false;
    QList<QSharedPointer<Chain>> toRemove;
    QList<QSharedPointer<Chain>> toAdd;
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

bool VideoNode::frozenInput() {
    QMutexLocker locker(&m_stateLock);
    return m_frozenInput;
}

void VideoNode::setFrozenInput(bool value) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if (value != m_frozenInput) { 
            m_frozenInput = value;
            changed = true;
        }
    }
    if (changed) emit frozenInputChanged(value);
}

bool VideoNode::frozenOutput() {
    QMutexLocker locker(&m_stateLock);
    return m_frozenOutput;
}

void VideoNode::setFrozenOutput(bool value) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if (value != m_frozenOutput) { 
            m_frozenOutput = value;
            changed = true;
        }
    }
    if (changed) emit frozenOutputChanged(value);
}

bool VideoNode::frozenParameters() {
    QMutexLocker locker(&m_stateLock);
    return m_frozenParameters;
}

void VideoNode::setFrozenParameters(bool value) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if (value != m_frozenParameters) { 
            m_frozenParameters = value;
            changed = true;
        }
    }
    if (changed) emit frozenParametersChanged(value);
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

QList<QSharedPointer<Chain>> VideoNode::requestedChains() {
    return QList<QSharedPointer<Chain>>();
}

void VideoNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
}

GLuint VideoNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
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

QDebug operator<<(QDebug debug, const VideoNode &vn)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "VideoNode";

    return debug;
}
