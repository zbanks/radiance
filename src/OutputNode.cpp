#include "OutputNode.h"
#include <QDebug>

OutputNode::OutputNode(NodeType *nr)
    : VideoNode(nr) {
}

OutputNode::OutputNode(const OutputNode &other)
    : VideoNode(other) {
}

OutputNode::~OutputNode() = default;

QString OutputNode::serialize() {
    return "its_an_output";
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
