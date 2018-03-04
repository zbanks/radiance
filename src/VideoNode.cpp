#include "VideoNode.h"
#include "Model.h"
#include <QtQml>

// d_ptr may be null here.
// Let the superclass initialize it
// and deal with construction of the VideoNodePrivate.
VideoNode::VideoNode(VideoNodePrivate *ptr)
    : d_ptr(ptr, &QObject::deleteLater)
{
}

VideoNode::VideoNode(const VideoNode &other)
    : d_ptr(other.d_ptr)
{
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
    if (changed) emit inputCountChanged(value);
}

QList<Chain> VideoNode::chains() {
    QMutexLocker locker(&d_ptr->m_stateLock);
    return d_ptr->m_chains;
}

void VideoNode::setChains(QList<Chain> chains) {
    bool wereChainsChanged = false;
    {
        QMutexLocker locker(&d_ptr->m_stateLock);
        QList<Chain> toRemove = d_ptr->m_chains;
        QList<Chain> toAdd;
        for (int i=0; i<chains.count(); i++) {
            if (d_ptr->m_chains.contains(chains.at(i))) {
                // If it exists already, don't remove it
                toRemove.removeAll(chains.at(i));
            } else {
                // If it doesn't exist already, add it
                toAdd.append(chains.at(i)); // Add it
            }
        }
        if (!toAdd.empty() || toRemove.empty()) {
            chainsEdited(toAdd, toRemove);
            d_ptr->m_chains = chains;
            wereChainsChanged = true;
        }
    }
    if (wereChainsChanged) emit chainsChanged(chains);
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
