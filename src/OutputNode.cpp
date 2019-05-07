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

// Only use this for promoting WeakOutputNodes
OutputNode::OutputNode(QSharedPointer<OutputNodePrivate> ptr)
    : VideoNode(ptr.staticCast<VideoNodePrivate>())
{
}

OutputNode *OutputNode::clone() const {
    return new OutputNode(*this);
}

QSharedPointer<OutputNodePrivate> OutputNode::d() const {
    return d_ptr.staticCast<OutputNodePrivate>();
}

QList<ChainSP> OutputNode::requestedChains() {
    auto l = QList<ChainSP>();
    l.append(chain());
    return l;
}

GLuint OutputNode::paint(ChainSP chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    return inputTextures.at(0);
}

GLuint OutputNode::render() {
    return render(lastModel());
}

GLuint OutputNode::render(WeakModel model) {
    auto modelCopy = model.createCopyForRendering();
    auto result = modelCopy.render(chain());
    return result.value(*this, 0);
}

ChainSP OutputNode::chain() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_chain;
}

void OutputNode::resize(QSize size) {
    ChainSP oldChain;
    ChainSP newChain;
    {
        QMutexLocker locker(&d()->m_stateLock);
        oldChain = d()->m_chain;
        if (size == oldChain->size()) return;
        newChain = ChainSP(new Chain(oldChain.data(), size));
        d()->m_chain = newChain;
    }
    emit requestedChainAdded(newChain);
    emit requestedChainRemoved(oldChain);
}

OutputNodePrivate::OutputNodePrivate(Context *context, QSize chainSize)
    : VideoNodePrivate(context)
    , m_chain(new Chain(chainSize))
{
}
