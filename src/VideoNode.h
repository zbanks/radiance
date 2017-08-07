#pragma once

#include "RenderContext.h"
#include <QObject>
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QOpenGLFunctions>
#include <QMutex>

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
    GLuint texture(int chain);

    // Returns the framebuffer size of the given chain
    QSize size(int chain);

    // Returns the render context
    RenderContext *context();

    // VideoNodes are created and managed from Javascript
    // but they are used in C++
    // These methods act as a reference counter for C++ references
    // so that we don't accidentally tell Javascript
    // that a VideoNode can be deleted
    // if it is still in use.
    void ref();
    void deRef();

signals:
    void noMoreRef(VideoNode *videoNode);

protected:
    RenderContext *m_context;
    int m_inputCount;
    QVector<GLuint> m_textures;
    QAtomicInt m_refCount;

signals:
    void deleteMe();
};
