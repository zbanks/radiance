#pragma once

#include "VideoNode.h"
#include "NodeType.h"
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <vector>

class OutputNode;
class OutputType;

class OutputType : public NodeType {
    Q_OBJECT
public:
    OutputType(NodeRegistry *r = nullptr, QObject *p = nullptr);
   ~OutputType() override;
public slots:
    VideoNode *create(QString) override;
};

///////////////////////////////////////////////////////////////////////////////

// This abstract class extends VideoNode to provide radiance output functionality.
// You should extend it if you are writing an output.

class OutputNode
    : public VideoNode {
    Q_OBJECT

    friend class OutputNodeOpenGLWorker;

public:
    OutputNode(NodeType *nr);
    OutputNode(const OutputNode &other);
    ~OutputNode();

    QString serialize() override;

    QSharedPointer<VideoNode> createCopyForRendering(QSharedPointer<Chain>) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

protected:
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;
};
