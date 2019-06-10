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
    o["role"] = role();
    // TODO serialize the wrapped VideoNode??
    return o;
}

void PlaceholderNode::setWrappedVideoNode(VideoNodeSP *wrapped) {
    {
        QMutexLocker locker(&m_stateLock);
        if (m_wrappedVideoNode != nullptr) {
            if (m_wrappedVideoNode->parent() == this) {
                delete m_wrappedVideoNode;
            }
        }
        m_wrappedVideoNode = wrapped;
        if (m_wrappedVideoNode != nullptr) {
            (*m_wrappedVideoNode)->setChains(m_chains); // XXX this needs to be more dynamic
        }
    }
    emit wrappedVideoNodeChanged(m_wrappedVideoNode);
}

VideoNodeSP *PlaceholderNode::wrappedVideoNode() {
    QMutexLocker locker(&m_stateLock);
    return m_wrappedVideoNode;
}

QString PlaceholderNode::role() {
    QMutexLocker locker(&m_stateLock);
    return m_role;
}

void PlaceholderNode::setRole(QString value) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if (value != m_role) { 
            m_role = value;
            changed = true;
        }
    }
    if (changed) emit roleChanged(value);
}

GLuint PlaceholderNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    QSharedPointer<VideoNode> wrapped;
    {
        QMutexLocker locker(&m_stateLock);

        if (m_wrappedVideoNode == nullptr) {
            if (inputTextures.size() >= 1) {
                return inputTextures.at(0);
            }
            return chain->blankTexture();
        }

        wrapped = qSharedPointerCast<VideoNode>(*m_wrappedVideoNode);
    }

    if (inputTextures.size() > wrapped->inputCount()) {
        inputTextures.resize(wrapped->inputCount());
    }
    while (inputTextures.size() < wrapped->inputCount()) {
        inputTextures.append(chain->blankTexture());
    }
    return wrapped->paint(chain, inputTextures);
}

void PlaceholderNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
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

    QString role;
    if (obj.contains("role")) {
        role = obj["role"].toString();
    }

    auto node = new PlaceholderNodeSP(new PlaceholderNode(context));
    (*node)->setInputCount(inputCount);
    (*node)->setRole(role);
    return node;
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
