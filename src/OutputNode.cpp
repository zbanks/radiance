#include "OutputNode.h"
#include <QDebug>
#include <QJsonObject>


OutputNode::OutputNode(Context *context, QSize chainSize)
    : VideoNode(new OutputNodePrivate(context, chainSize))
{
    setInputCount(1);
}

OutputNode::OutputNode(OutputNodePrivate *ptr)
    : VideoNode(ptr)
{
    setInputCount(1);
}

OutputNode::OutputNode(const OutputNode &other)
    : VideoNode(other)
{
}

OutputNode *OutputNode::clone() const {
    return new OutputNode(*this);
}

QSharedPointer<OutputNodePrivate> OutputNode::d() {
    return d_ptr.staticCast<OutputNodePrivate>();
}

QList<Chain> OutputNode::requestedChains() {
    auto l = QList<Chain>();
    l.append(chain());
    return l;
}

GLuint OutputNode::paint(Chain chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    return inputTextures.at(0);
}

GLuint OutputNode::render(Model *model) {
    if (model == nullptr) {
        qDebug() << "Parent is" << parent();
        // This is a little bit of a hack,
        model = qobject_cast<Model*>(parent());
        if (model == nullptr) {
            return 0;
        }
    }
    auto modelCopy = model->createCopyForRendering();
    auto result = modelCopy.render(chain());
    qDebug() << "Rendered" << result;
    qDebug() << "(I am" << *this << ")";
    return result.value(*this, 0);
}

Chain OutputNode::chain() {
    // Don't need to lock as long as m_chain is immutable
    return d()->m_chain;
}

OutputNodePrivate::OutputNodePrivate(Context *context, QSize chainSize)
    : VideoNodePrivate(context)
    , m_chain(chainSize)
{
}
