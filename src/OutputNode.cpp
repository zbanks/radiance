#include "OutputNode.h"
#include <QDebug>
#include <QJsonObject>

OutputNode::OutputNode(Context *context, QSize chainSize)
    : VideoNode(context)
    , m_chain(new Chain(chainSize), &QObject::deleteLater)
{
    setInputCount(1);
}

QList<QSharedPointer<Chain>> OutputNode::requestedChains() {
    auto l = QList<QSharedPointer<Chain>>();
    l.append(chain());
    return l;
}

GLuint OutputNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    return inputTextures.at(0);
}

GLuint OutputNode::render() {
    return render(lastModel());
}

GLuint OutputNode::render(QWeakPointer<Model> model) {
    auto modelCopy = Model::createCopyForRendering(model);
    auto result = modelCopy.render(chain());
    return result.value(qSharedPointerCast<VideoNode>(sharedFromThis()), 0);
}

void OutputNode::setWorkerContext(OpenGLWorkerContext *context) {
    m_workerContext = context;
    if (m_workerContext != nullptr) {
        m_chain->moveToWorkerContext(m_workerContext);
    }
}

QSharedPointer<Chain> OutputNode::chain() {
    QMutexLocker locker(&m_stateLock);
    return m_chain;
}

void OutputNode::resize(QSize size) {
    QSharedPointer<Chain> oldChain;
    QSharedPointer<Chain> newChain;
    {
        QMutexLocker locker(&m_stateLock);
        oldChain = m_chain;
        if (size == oldChain->size()) return;
        newChain = QSharedPointer<Chain>(new Chain(oldChain.data(), size), &QObject::deleteLater);
        m_chain = newChain;
        if (m_workerContext != nullptr) {
            m_chain->moveToWorkerContext(m_workerContext);
        }
    }
    emit requestedChainAdded(newChain);
    emit requestedChainRemoved(oldChain);
}
