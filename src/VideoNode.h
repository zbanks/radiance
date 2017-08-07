#pragma once

#include "RenderContext.h"
#include <QObject>
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QOpenGLFunctions>
#include <QMutex>

class Model;

struct VideoNodeRenderState {
public:
    int m_texture;
};

// VideoNodes have 0 or more inputs, and one output.

class VideoNode : public QObject {
    Q_OBJECT

public:
    // After the constructor has run,
    // m_inputCount must be at its final value.
    // Calls to paint() may return nullptr
    // before initialize() has been run.
    // Constructor may be called without a valid
    // OpenGL context.
    VideoNode(RenderContext *context, int inputCount = 0);

    // VideoNodes must be copyable
    VideoNode(const VideoNode &other);

   ~VideoNode() override;

    // Paint is run from a valid OpenGL context.
    // It should update all the framebuffers,
    // and set m_texture to the QOpenGLTexture
    // that serves as the output.
    // This texture must remain valid
    // _throughout_ the _next_ call to paint,
    // i.e. nodes should take care to double-buffer properly.

    // The input vector is a list of textures from the previous inputs
    // and is guaranteed to be non-null and have .count() == m_inputCount
    // and size corresponding to the chain.
    // These textures must not be written to.
    virtual void paint(int chain, QVector<GLuint> inputTextures) = 0;

    // Must not change after constructor
    int inputCount();

    // Returns the output texture
    // for the given chain
    // or 0 if the node is not ready for any reason
    virtual GLuint texture(int chain) = 0;

    // Returns the framebuffer size of the given chain
    QSize size(int chain);

    // Returns the render context
    RenderContext *context();

    // Creates a copy of this node
    virtual QSharedPointer<VideoNode> createCopyForRendering() = 0;

    // Reads back the new render state
    virtual void copyBackRenderState(int chain, QSharedPointer<VideoNode> copy) = 0;

protected:
    RenderContext *m_context;
    int m_inputCount;
    QMutex m_stateLock;

signals:
    void deleteMe();
};
