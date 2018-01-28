#pragma once

#include "VideoNode.h"
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
    OutputNode(Context *context);
    OutputNode(const OutputNode &other);
    ~OutputNode();

    QJsonObject serialize() override;

    QSharedPointer<VideoNode> createCopyForRendering(QSharedPointer<Chain>) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

protected:
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;
};
