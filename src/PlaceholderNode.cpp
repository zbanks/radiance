#include "PlaceholderNode.h"
#include <QDebug>
#include <QJsonObject>

PlaceholderNode::PlaceholderNode(Context *context, VideoNodeSP *wrapped)
    : VideoNode(context) {

    setInputCount(1);
    setWrappedVideoNode(wrapped);
}

QJsonObject PlaceholderNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o["inputCount"] = inputCount();
    // TODO serialize the wrapped VideoNode??
    return o;
}

void PlaceholderNode::setWrappedVideoNode(VideoNodeSP *wrapped) {
    {
        QMutexLocker locker(&m_stateLock);
        if (wrapped != nullptr) {
            wrapped = new VideoNodeSP(*wrapped);
            (*wrapped)->setChains(m_chains);
        }
        m_wrappedVideoNode = QSharedPointer<VideoNodeSP>(wrapped);
    }
    emit wrappedVideoNodeChanged(wrapped);
}

VideoNodeSP *PlaceholderNode::wrappedVideoNode() {
    QMutexLocker locker(&m_stateLock);
    return m_wrappedVideoNode.data();
}

GLuint PlaceholderNode::paint(ChainSP chain, QVector<GLuint> inputTextures) {
    QSharedPointer<VideoNodeSP> wrapped; // How many layers of QSP can we have
    {
        QMutexLocker locker(&m_stateLock);
        wrapped = m_wrappedVideoNode;
    }

    if (wrapped.isNull()) {
        if (inputTextures.size() >= 1) {
            return inputTextures.at(0);
        }
        return chain->blankTexture();
    }
    if (inputTextures.size() > (*wrapped)->inputCount()) {
        inputTextures.resize((*wrapped)->inputCount());
    }
    while (inputTextures.size() < (*wrapped)->inputCount()) {
        inputTextures.append(chain->blankTexture());
    }
    return (*wrapped)->paint(chain, inputTextures);
}

void PlaceholderNode::chainsEdited(QList<ChainSP> added, QList<ChainSP> removed) {
    if (m_wrappedVideoNode == nullptr) {
        return;
    }

    (*m_wrappedVideoNode)->setChains(chains());
}

QString PlaceholderNode::typeName() {
    return "PlaceholderNode";
}

VideoNodeSP *PlaceholderNode::deserialize(Context *context, QJsonObject obj) {
    if (obj.isEmpty()) {
        return nullptr;
    }

    int inputCount = 1;
    QJsonValue inputCountValue = obj["inputCount"];
    if (inputCountValue.isDouble()) {
        inputCount = inputCountValue.toInt();
    }

    auto e = new PlaceholderNode(context);
    e->setInputCount(inputCount);
    return new VideoNodeSP(e);
}

bool PlaceholderNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNodeSP *PlaceholderNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> PlaceholderNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("Placeholder", "PlaceholderInstantiator.qml");
    return m;
}
