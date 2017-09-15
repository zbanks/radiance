#include "Output.h"

Output::Output() {
}

Output::~Output() {
}

QSharedPointer<Chain> Output::chain() {
    QMutexLocker locker(&m_chainLock);
    return m_chain;
}

void Output::setChain(QSharedPointer<Chain> chain) {
    bool changed = false;
    {
        QMutexLocker locker(&m_chainLock);
        if (m_chain != chain) {
            m_chain = chain;
            changed = true;
        }
    }
    if (changed) emit chainChanged(chain);
}

// This method is thread-safe
QString Output::name() {
    QMutexLocker locker(&m_nameLock);
    return m_name;
}

// This method is thread-safe
void Output::setName(QString name) {
    bool changed = false;
    {
        QMutexLocker locker(&m_nameLock);
        if (m_name != name) {
            m_name = name;
            changed = true;
        }
    }
    if (changed) emit nameChanged(name);
}
