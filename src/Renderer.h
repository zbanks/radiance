#pragma once

#include "Output.h"
#include <QList>

// This class is like an output manager I guess

class Renderer : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<Output *> outputs READ outputs NOTIFY outputsChanged)

public:
    Renderer();
   ~Renderer() override;
    QList<QSharedPointer<Chain>> chains();

public slots:
    QList<Output *> outputs();
    void addOutput(Output * output);
    void removeOutput(Output * output);

protected slots:
    void onRenderRequested();

signals:
    void outputsChanged(QList<Output *> outputs);
    void renderRequested(Output *output);

private:
    QList<Output *> m_outputs;
};
