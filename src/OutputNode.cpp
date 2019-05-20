#include "OutputNode.h"
#include <QDebug>
#include <QJsonObject>

OutputNode::OutputNode(Context *context, QSize chainSize)
    : VideoNode(context)
    , m_chain(new Chain(chainSize))
{
    setInputCount(1);
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

GLuint OutputNode::render(QWeakPointer<Model> model) {
    auto modelCopy = Model::createCopyForRendering(model);
    auto result = modelCopy.render(chain());
    return result.value(*this, 0);
}

ChainSP OutputNode::chain() {
    QMutexLocker locker(&m_stateLock);
    return m_chain;
}

void OutputNode::resize(QSize size) {
    ChainSP oldChain;
    ChainSP newChain;
    {
        QMutexLocker locker(&m_stateLock);
        oldChain = m_chain;
        if (size == oldChain->size()) return;
        newChain = ChainSP(new Chain(oldChain.data(), size), &QObject::deleteLater);
        m_chain = newChain;
    }
    emit requestedChainAdded(newChain);
    emit requestedChainRemoved(oldChain);
}
