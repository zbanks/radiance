#include "PlaceholderNode.h"
#include <QDebug>
#include <QJsonObject>

PlaceholderNode::PlaceholderNode(Context *context, VideoNode *wrapped)
    : VideoNode(context)
    , m_wrappedVideoNode(wrapped)
    , m_ownedWrappedVideoNode(nullptr) {
    setInputCount(1);
}

PlaceholderNode::PlaceholderNode(const PlaceholderNode &other, QSharedPointer<VideoNode> ownedWrapped)
    : VideoNode(other)
    , m_wrappedVideoNode(other.m_wrappedVideoNode)
    , m_ownedWrappedVideoNode(ownedWrapped) {
}

PlaceholderNode::~PlaceholderNode() = default;

QJsonObject PlaceholderNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o["inputCount"] = inputCount();
    return o;
}

void PlaceholderNode::setWrappedVideoNode(VideoNode *wrapped) {
    m_wrappedVideoNode = wrapped;
    m_wrappedVideoNode->setChains(chains());
}

// See comments in PlaceholderNode.h about these 3 functions
QSharedPointer<VideoNode> PlaceholderNode::createCopyForRendering(Chain chain) {
    if (m_wrappedVideoNode == nullptr) {
        return QSharedPointer<VideoNode>(new PlaceholderNode(*this));
    }
    QSharedPointer<VideoNode> wrappedCopy = m_wrappedVideoNode->createCopyForRendering(chain);
    return QSharedPointer<VideoNode>(new PlaceholderNode(*this, wrappedCopy));
}

GLuint PlaceholderNode::paint(Chain chain, QVector<GLuint> inputTextures) {
    if (m_wrappedVideoNode == nullptr) {
        if (inputTextures.size() >= 1) {
            return inputTextures.at(0);
        }
        return chain.blankTexture();
    }
    if (inputTextures.size() > m_wrappedVideoNode->inputCount()) {
        inputTextures.resize(m_wrappedVideoNode->inputCount());
    }
    while (inputTextures.size() < m_wrappedVideoNode->inputCount()) {
        inputTextures.append(chain.blankTexture());
    }
    return m_wrappedVideoNode->paint(chain, inputTextures);
}

void PlaceholderNode::chainsEdited(QList<Chain> added, QList<Chain> removed) {
    if (m_wrappedVideoNode == nullptr) {
        return;
    }

    m_wrappedVideoNode->setChains(chains());
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
