#pragma once

#include "VideoNode.h"
#include "Model.h"
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <vector>

// This abstract class extends VideoNode to provide radiance output functionality.
// You should extend it if you are writing an output.

class OutputNode
    : public VideoNode {
    Q_OBJECT

    friend class OutputNodeOpenGLWorker;

public:
    OutputNode(Context *context, QSize chainSize);
    OutputNode(const OutputNode &other);
    ~OutputNode();

    QSharedPointer<VideoNode> createCopyForRendering(QSharedPointer<Chain>) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;
    GLuint render(Model *model);

protected:
    virtual QList<QSharedPointer<Chain>> requestedChains();
    QSharedPointer<Chain> m_chain;
};
