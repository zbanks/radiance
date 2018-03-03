#pragma once

#include "VideoNode.h"
#include "Model.h"
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <vector>

// This abstract class extends VideoNode to provide radiance output functionality.
// You should extend it if you are writing an output.

class OutputNodePrivate;

class OutputNode : public VideoNode {
    Q_OBJECT

public:
    OutputNode(Context *context, QSize chainSize);
    OutputNode(const OutputNode &other);
    OutputNode *clone() const override;
    OutputNode(OutputNodePrivate *ptr);

    GLuint paint(Chain chain, QVector<GLuint> inputTextures) override;
    GLuint render();
    GLuint render(WeakModel model);

    Chain chain();

protected:
    virtual QList<Chain> requestedChains() override;

private:
    QSharedPointer<OutputNodePrivate> d();
};

class OutputNodePrivate : public VideoNodePrivate {
    Q_OBJECT

public:
    OutputNodePrivate(Context *context, QSize chainSize);
    Chain m_chain;
};
