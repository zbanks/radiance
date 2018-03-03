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

public:
    OutputNode(Context *context, QSize chainSize);
    OutputNode(const OutputNode &other);
    ~OutputNode();

    QSharedPointer<VideoNode> createCopyForRendering(Chain chain) override;
    GLuint paint(Chain chain, QVector<GLuint> inputTextures) override;
    GLuint render(Model *model = nullptr);

    Chain chain();

protected:
    virtual QList<Chain> requestedChains() override;
    Chain m_chain;
};
