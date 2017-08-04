#pragma once

#include "RenderContext.h"
#include <QObject>
#include <QOpenGLTexture>
#include <QSharedPointer>

class Model;

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

   ~VideoNode() override;

    // Initialize is called with a valid
    // OpenGL context.
    // Initialize may call deleteLater(this)
    // if a fatal error occurred.
    virtual void initialize() = 0;

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
    virtual void paint(int chain, QVector<QSharedPointer<QOpenGLTexture> > inputTextures) = 0;

    // Must not change after constructor
    int inputCount();

    // Returns the output texture
    // for the given chain
    // or nullptr if the node is not ready for any reason
    QSharedPointer<QOpenGLTexture> texture(int chain);

protected:
    RenderContext *m_context;
    int m_inputCount;
    QVector<QSharedPointer<QOpenGLTexture> > m_textures;

signals:
    void deleteMe();
};
