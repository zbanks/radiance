#include "VideoNode.h"
#include "Model.h"
#include <QtQml>

// d_ptr may be null here.
// Let the superclass initialize it
// and deal with construction of the VideoNodePrivate.
VideoNode::VideoNode(VideoNodePrivate *ptr)
    : d_ptr(ptr, &QObject::deleteLater)
{
    attachSignals();
}

// Only use this for promoting WeakVideoNodes
VideoNode::VideoNode(QSharedPointer<VideoNodePrivate> ptr)
    : d_ptr(ptr)
{
    attachSignals();
}

VideoNode::VideoNode(const VideoNode &other)
    : d_ptr(other.d_ptr)
{
    attachSignals();
}

void VideoNode::attachSignals() {
    // TODO I think this can be done with QMetaObject
    // which will remove the need to have this method
    // in subclasses
    qRegisterMetaType<Chain>("Chain"); // so we can pass it in Q_ARG
    qRegisterMetaType<QList<Chain>>("QList<Chain>");
    qRegisterMetaType<VideoNode::NodeState>("VideoNode::NodeState");

    connect(d_ptr.data(), &VideoNodePrivate::message, this, &VideoNode::message);
    connect(d_ptr.data(), &VideoNodePrivate::warning, this, &VideoNode::warning);
    connect(d_ptr.data(), &VideoNodePrivate::error, this, &VideoNode::error);
    connect(d_ptr.data(), &VideoNodePrivate::inputCountChanged, this, &VideoNode::inputCountChanged);
    connect(d_ptr.data(), &VideoNodePrivate::chainsChanged, this, &VideoNode::chainsChanged);
    connect(d_ptr.data(), &VideoNodePrivate::requestedChainAdded, this, &VideoNode::requestedChainAdded);
    connect(d_ptr.data(), &VideoNodePrivate::requestedChainRemoved, this, &VideoNode::requestedChainRemoved);
    connect(d_ptr.data(), &VideoNodePrivate::nodeStateChanged, this, &VideoNode::nodeStateChanged);
}

VideoNode *VideoNode::clone() const {
    return new VideoNode(*this);
}

bool VideoNode::operator==(const VideoNode &other) const {
    return d_ptr == other.d_ptr;
}

bool VideoNode::operator>(const VideoNode &other) const {
    return d_ptr.data() > other.d_ptr.data();
}

bool VideoNode::operator<(const VideoNode &other) const {
    return d_ptr.data() > other.d_ptr.data();
}

VideoNode &VideoNode::operator=(const VideoNode &other) {
    d_ptr = other.d_ptr;
    return *this;
}

VideoNode::operator QString() const {
    return QString("%1(d_ptr=%2)").arg(metaObject()->className()).arg(QString().sprintf("%p", d_ptr.data()));
}

int VideoNode::inputCount() {
    QMutexLocker locker(&d_ptr->m_stateLock);
    return d_ptr->m_inputCount;
}

void VideoNode::setInputCount(int value) {
    bool changed = false;
    {
        QMutexLocker locker(&d_ptr->m_stateLock);
        if (value != d_ptr->m_inputCount) { 
            d_ptr->m_inputCount = value;
            changed = true;
        }
    }
    if (changed) emit d_ptr->inputCountChanged(value);
}

VideoNode::NodeState VideoNode::nodeState() {
    QMutexLocker locker(&d_ptr->m_stateLock);
    return d_ptr->m_nodeState;
}

void VideoNode::setNodeState(NodeState value) {
    bool changed = false;
    {
        QMutexLocker locker(&d_ptr->m_stateLock);
        if (value != d_ptr->m_nodeState) { 
            d_ptr->m_nodeState = value;
            changed = true;
        }
    }
    if (changed) emit d_ptr->nodeStateChanged(value);
}

QList<Chain> VideoNode::chains() {
    QMutexLocker locker(&d_ptr->m_stateLock);
    return d_ptr->m_chains;
}

void VideoNode::setChains(QList<Chain> chains) {
    bool wereChainsChanged = false;
    QList<Chain> toRemove;
    QList<Chain> toAdd;
    {
        QMutexLocker locker(&d_ptr->m_stateLock);
        toRemove = d_ptr->m_chains;
        for (int i=0; i<chains.count(); i++) {
            if (d_ptr->m_chains.contains(chains.at(i))) {
                // If it exists already, don't remove it
                toRemove.removeAll(chains.at(i));
            } else {
                // If it doesn't exist already, add it
                toAdd.append(chains.at(i)); // Add it
            }
        }
        if (!toAdd.empty() || !toRemove.empty()) {
            d_ptr->m_chains = chains;
            wereChainsChanged = true;
        }
    }
    if (wereChainsChanged) {
        chainsEdited(toAdd, toRemove);
        emit d_ptr->chainsChanged(chains);
    }
}

Context *VideoNode::context() {
    // Not mutable, so no need to lock
    return d_ptr->m_context;
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

GLuint VideoNode::paint(Chain chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    Q_UNUSED(inputTextures);
    return 0;
}

void VideoNode::setLastModel(WeakModel model) {
    QMutexLocker locker(&d_ptr->m_stateLock);
    d_ptr->m_lastModel = model;
}

WeakModel VideoNode::lastModel() {
    QMutexLocker locker(&d_ptr->m_stateLock);
    return d_ptr->m_lastModel;
}

VideoNodePrivate::VideoNodePrivate(Context *context)
    : m_context(context)
{
}
