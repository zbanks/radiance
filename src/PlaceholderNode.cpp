#include "PlaceholderNode.h"
#include <QDebug>
#include <QJsonObject>

PlaceholderNode::PlaceholderNode(Context *context, VideoNode *wrapped)
    : VideoNode(new PlaceholderNodePrivate(context)) {

    setInputCount(1);
    setWrappedVideoNode(wrapped);
}

PlaceholderNode::PlaceholderNode(const PlaceholderNode &other)
    : VideoNode(other)
{
}

PlaceholderNode *PlaceholderNode::clone() const {
    return new PlaceholderNode(*this);
}

QSharedPointer<PlaceholderNodePrivate> PlaceholderNode::d() {
    return d_ptr.staticCast<PlaceholderNodePrivate>();
}

QJsonObject PlaceholderNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o["inputCount"] = inputCount();
    // TODO serialize the wrapped VideoNode??
    return o;
}

void PlaceholderNode::setWrappedVideoNode(VideoNode *wrapped) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        if (wrapped != nullptr) {
            wrapped = wrapped->clone();
            wrapped->setChains(d()->m_chains);
        }
        d()->m_wrappedVideoNode = QSharedPointer<VideoNode>(wrapped);
    }
    emit wrappedVideoNodeChanged(wrapped);
}

VideoNode *PlaceholderNode::wrappedVideoNode() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_wrappedVideoNode.data();
}

GLuint PlaceholderNode::paint(ChainSP chain, QVector<GLuint> inputTextures) {
    QSharedPointer<VideoNode> wrapped; // How many layers of QSP can we have
    {
        QMutexLocker locker(&d()->m_stateLock);
        wrapped = d()->m_wrappedVideoNode;
    }

    if (wrapped.isNull()) {
        if (inputTextures.size() >= 1) {
            return inputTextures.at(0);
        }
        return chain->blankTexture();
    }
    if (inputTextures.size() > wrapped->inputCount()) {
        inputTextures.resize(wrapped->inputCount());
    }
    while (inputTextures.size() < wrapped->inputCount()) {
        inputTextures.append(chain->blankTexture());
    }
    return wrapped->paint(chain, inputTextures);
}

void PlaceholderNode::chainsEdited(QList<ChainSP> added, QList<ChainSP> removed) {
    if (d()->m_wrappedVideoNode == nullptr) {
        return;
    }

    d()->m_wrappedVideoNode->setChains(chains());
}

QString PlaceholderNode::typeName() {
    return "PlaceholderNode";
}

VideoNode *PlaceholderNode::deserialize(Context *context, QJsonObject obj) {
    if (obj.isEmpty()) {
        return nullptr;
    }

    int inputCount = 1;
    QJsonValue inputCountValue = obj["inputCount"];
    if (inputCountValue.isDouble()) {
        inputCount = inputCountValue.toInt();
    }

    PlaceholderNode *e = new PlaceholderNode(context);
    e->setInputCount(inputCount);
    return e;
}

bool PlaceholderNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *PlaceholderNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> PlaceholderNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("Placeholder", "PlaceholderInstantiator.qml");
    return m;
}

PlaceholderNodePrivate::PlaceholderNodePrivate(Context *context)
    : VideoNodePrivate(context)
{
}
