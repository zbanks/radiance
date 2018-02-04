#include "OutputNode.h"
#include <QDebug>
#include <QJsonObject>

OutputNode::OutputNode(Context *context, QSize chainSize)
    : VideoNode(context)
    , m_chain(QSharedPointer<Chain>(new Chain(chainSize))) {
    m_inputCount = 1;
}

OutputNode::OutputNode(const OutputNode &other)
    : VideoNode(other) {
    m_inputCount = other.m_inputCount;
}

OutputNode::~OutputNode() = default;

QList<QSharedPointer<Chain>> OutputNode::requestedChains() {
    auto l = QList<QSharedPointer<Chain>>();
    l.append(m_chain);
    return l;
}

QSharedPointer<VideoNode> OutputNode::createCopyForRendering(QSharedPointer<Chain> chain) {
    Q_UNUSED(chain);
    return QSharedPointer<VideoNode>(new OutputNode(*this));
}

GLuint OutputNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    return inputTextures.at(0);
}

GLuint OutputNode::render(Model *model) {
    if (model == nullptr) {
        // This is a little bit of a hack,
        model = qobject_cast<Model*>(parent());
        if (model == nullptr) {
            return 0;
        }
    }
    auto modelCopy = model->createCopyForRendering(m_chain);
    auto result = modelCopy.render(m_chain);
    return result.value(id(), 0);
}

QSharedPointer<Chain> OutputNode::chain() {
    return m_chain;
}
