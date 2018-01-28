#include "OutputNode.h"
#include <QDebug>
#include <QJsonObject>

OutputNode::OutputNode(Context *context)
    : VideoNode(context) {
}

OutputNode::OutputNode(const OutputNode &other)
    : VideoNode(other) {
}

OutputNode::~OutputNode() = default;

QJsonObject OutputNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    // TODO add things here
    return o;
}

void OutputNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
}

QSharedPointer<VideoNode> OutputNode::createCopyForRendering(QSharedPointer<Chain> chain) {
    Q_UNUSED(chain);
    return QSharedPointer<VideoNode>(new OutputNode(*this));
}

GLuint OutputNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    return inputTextures.at(0);
}
