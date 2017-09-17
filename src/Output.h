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

    // This method wraps display()
    // but takes the renderLock
    // so that subclasses can't possibly fuck it up
    void renderReady(GLuint texture);

    // This method is called when the render is finished
    // and there is a result to display.
    // It should be called from the same thread that renderRequested
    // was emitted from.
    // This method runs with the renderLock claimed.
    virtual void display(GLuint texture) = 0;

public slots:
    // Convenience function to request a render
    // from a signal that takes no arguments
    void requestRender();

signals:
    // This signal is emitted when this output wants something to be rendered
    // It is recommended to only make direct connections
    // to this signal, synchronously render,
    // and directly call display when done.
    // It should be called with `this` as an argument
    // so that the receiver knows where the render request
    // is coming from
    void renderRequested(Output *output);

    void chainChanged(QSharedPointer<Chain> chain);
    void nameChanged(QString name);

protected:
    // This lock should be taken during rendering
    // to ensure that name and chain do not change
    // and that the node is not deleted during rendering.
    QMutex m_renderLock;
    QSharedPointer<Chain> m_chain;
    QString m_name;
};
