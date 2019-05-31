#pragma once

#include "VideoNode.h"
#include "Model.h"
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <vector>

// This abstract class extends VideoNode to provide radiance output functionality.
// You should extend it if you are writing an output.

class OutputNode : public VideoNode {
    Q_OBJECT

public:
    OutputNode(Context *context, QSize chainSize);

    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;
    GLuint render();
    GLuint render(QWeakPointer<Model> model);

public slots:
    void resize(QSize size);

    QSharedPointer<Chain> chain();

protected:
    virtual QList<QSharedPointer<Chain>> requestedChains() override;

    QSharedPointer<Chain> m_chain;
};

typedef QmlSharedPointer<OutputNode, VideoNodeSP> OutputNodeSP;
Q_DECLARE_METATYPE(OutputNodeSP*)
