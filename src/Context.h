#pragma once

#include "Output.h"
#include "Model.h"
#include <QList>

class Context : public QObject {
    Q_OBJECT
    Q_PROPERTY(Model *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QList<Output *> outputs READ outputs WRITE setOutputs NOTIFY outputsChanged)
    Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize NOTIFY previewSizeChanged)

public:
    Context(bool hasPreview=true);
   ~Context() override;

public slots:
    Model *model();
    void setModel(Model *model);
    QSize previewSize();
    void setPreviewSize(QSize size);

    QList<Output *> outputs();
    void setOutputs(QList<Output *> outputs);

    // Call this method to render the preview chain
    void previewRenderRequested();

protected slots:
    void onRenderRequested();

signals:
    void previewSizeChanged(QSize size);
    void outputsChanged(QList<Output *> outputs);
    void modelChanged(Model *model);

    // This signal is emitted
    // once the preview has finished rendering.
    // It should be DirectConnected to your
    // preview's display() slot.
    void previewRendered(QMap<int, GLuint>);

protected:
    void chainsChanged();
    QList<QSharedPointer<Chain>> chains();

    Model *m_model;
    QList<Output *> m_outputs;
    bool m_hasPreview;
    QSize m_previewSize;
    QSharedPointer<Chain> m_previewChain;
};
