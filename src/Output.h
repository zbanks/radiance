#pragma once

#include "Chain.h"

class Output : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    Output();
   ~Output() override;

    // Returns the chain corresponding to this output
    // This method is thread-safe
    QSharedPointer<Chain> chain();

    // This method is thread-safe
    void setChain(QSharedPointer<Chain> chain);

    // This method is thread-safe
    QString name();

    // This method is thread-safe
    void setName(QString name);

    // This method is called when the render is finished
    // and there is a result to display.
    // It should be called from the same thread that renderRequested
    // was emitted from.
    // All methods that your implementation of display() uses
    // must be thread-safe.
    virtual void display(QMap<int, GLuint> result) = 0;

signals:
    // This signal is emitted when this output wants something to be rendered
    // It is recommended to only make direct connections
    // to this signal, synchronously render,
    // and directly call display when done.
    void renderRequested();

    void chainChanged(QSharedPointer<Chain> chain);
    void nameChanged(QString name);

protected:
    QMutex m_chainLock;
    QMutex m_nameLock;
    QSharedPointer<Chain> m_chain;
    QString m_name;
};
