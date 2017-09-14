#pragma once

#include "Chain.h"

class Output : public QObject {
    Q_OBJECT

public:
    Output();
   ~Output() override;

    // Returns the chain corresponding to this output
    virtual QSharedPointer<Chain> chain() = 0;

    // This method is called when the render is finished
    // and there is a result to display.
    virtual void display(QMap<int, GLuint> result) = 0;

signals:
    // This signal is emitted when this output wants something to be rendered
    // It is recommended to only make direct connections
    // to this signal, synchronously render,
    // and directly call onRender when done.
    void renderRequested();
};
