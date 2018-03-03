#include "OutputNode.h"
#include <QDebug>
#include <QJsonObject>

OutputNode::OutputNode(Context *context, QSize chainSize)
    : VideoNode(context)
    , m_chain(chainSize) {
    m_inputCount = 1;
}

OutputNode::OutputNode(const OutputNode &other)
    : VideoNode(other)
    , m_chain(other.m_chain)
{
    m_inputCount = other.m_inputCount;
}

OutputNode::~OutputNode() = default;

QList<Chain> OutputNode::requestedChains() {
    auto l = QList<Chain>();
    l.append(m_chain);
    return l;
}

QSharedPointer<VideoNode> OutputNode::createCopyForRendering(Chain chain) {
    Q_UNUSED(chain);
    return QSharedPointer<VideoNode>(new OutputNode(*this));
}

GLuint OutputNode::paint(Chain chain, QVector<GLuint> inputTextures) {
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
    // XXX
    // Don't we need to copy back the render state here?
    // Where did that function go?
    return result.value(id(), 0);
}

Chain OutputNode::chain() {
    return m_chain;
}
