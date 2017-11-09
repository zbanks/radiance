#include "Output.h"

Output::Output() {
}

Output::~Output() {
}

QSharedPointer<Chain> Output::chain() {
    QMutexLocker locker(&m_renderLock);
    return m_chain;
}

void Output::setChain(QSharedPointer<Chain> chain) {
    {
        QMutexLocker locker(&m_renderLock);
        if (m_chain == chain)
            return;
        m_chain = chain;
    }
    emit chainChanged(chain);
}

// This method is thread-safe
QString Output::name() {
    QMutexLocker locker(&m_renderLock);
    return m_name;
}

// This method is thread-safe
void Output::setName(QString name) {
    {
        QMutexLocker locker(&m_renderLock);
        if (m_name == name)
            return;
        m_name = name;
    }
    emit nameChanged(name);
}

void Output::requestRender() {
    emit renderRequested(this);
}

// This method is thread-safe
void Output::renderReady(GLuint texture) {
    QMutexLocker locker(&m_renderLock);
    display(texture);
}
