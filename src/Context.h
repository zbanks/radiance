#pragma once

#include "Output.h"
#include "Renderer.h"
#include "Model.h"
#include <QList>

// This class is like an output manager I guess

class Context : public QObject {
    Q_OBJECT
    Q_PROPERTY(Model *model READ model)
    Q_PROPERTY(Renderer *renderer READ renderer)

public:
    Context();
   ~Context() override;

public slots:
    Model *model();
    Renderer *renderer();

protected slots:
    void onRenderRequested(Output *output);
    void onOutputsChanged(QList<Output *> outputs);

private:
    QMap<int, GLuint> render(QSharedPointer<Chain> chain);

    Model m_model;
    Renderer m_renderer;
};
