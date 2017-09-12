#pragma once

#include "RenderContext.h"
#include <QObject>
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QOpenGLFunctions>
#include <QMutex>

class Model;

// This is an abstract base class
// for nodes in the DAG.
// VideoNodes have 0 or more inputs, and one output.
// Unless explicitly stated otherwise,
// VideoNode methods are not thread-safe
// and should be called in the same thread
// or through queued signals and slots.
// The few exceptions to this
// are methods relating to rendering.

class VideoNode : public QObject {
    Q_OBJECT
    Q_PROPERTY(int inputCount READ inputCount WRITE setInputCount NOTIFY inputCountChanged);
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged);
    Q_PROPERTY(int id READ id);

public:
    // Calls to paint() may return nullptr
    // before initialize() has been run.
    // Constructor may be called without a valid
    // OpenGL context.
    VideoNode(QSharedPointer<RenderContext> context);

    // VideoNodes must be copyable
    VideoNode(const VideoNode &other);

   ~VideoNode() override;

    // Returns the render context
    QSharedPointer<RenderContext> context();

    // Methods for rendering

    // Creates a copy of this node
    // This function is thread-safe
    // and the ability to run this function from any thread
    // is why any state mutation
    // needs to take the stateLock.
    // The new object will be assigned to the thread
    // that createCopyForRendering is called in.
    virtual QSharedPointer<VideoNode> createCopyForRendering() = 0;

    // Paint is run from a valid OpenGL context.
    // It should return the OpenGL texture ID
    // that serves as the output,
    // or 0 if the node is not ready for any reason.
    // This texture must remain valid
    // _throughout_ the _next_ call to paint,
    // i.e. nodes should take care to double-buffer properly.

    // The input vector is a list of textures from the previous inputs
    // and is guaranteed to be non-null and have .count() == m_inputCount
    // and size corresponding to the chain.
    // These textures must not be written to.

    // This function is NOT thread-safe.
    // Since rendering is almost always done on a different thread,
    // a copy of the VideoNode object must be created.
    // paint() may mutate the RenderState
    // for the chain that it was called on.
    // The new RenderState can be propogated back
    // to the original VideoNode object
    // using the thread-safe method copyBackRenderState.
    virtual GLuint paint(int chain, QVector<GLuint> inputTextures) = 0;

    // Copies back the new render state into the original object
    // This function is thread safe.
    // Since only a single RenderState entry is copied back,
    // only operations that access (or mutate) the RenderState
    // need to take the stateLock.
    virtual void copyBackRenderState(int chain, QSharedPointer<VideoNode> copy) = 0;

public slots:
    // Number of inputs
    int inputCount();
    void setInputCount(int value);

    // If the VideoNode is ready to go
    bool ready();

    // Returns the framebuffer size of the given chain
    // This method is thread-safe
    // because it is accessing an element of RenderContext
    // which will not change during this node's lifetime
    QSize size(int chain);

    // Returns a unique identifier
    // within the render context
    VnId id();

protected slots:
    void setReady(bool value);

protected:
    QSharedPointer<RenderContext> m_context;
    int m_inputCount;
    bool m_ready;
    QMutex m_stateLock;
    VnId m_id;

signals:
    // Emitted when the object wishes to be deleted
    // e.g. due to an error
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);

    // Emitted when the number of inputs changes
    void inputCountChanged(int value);

    // Emitted when the ready state changes
    void readyChanged(bool value);
};

class VideoNodeReference : public QObject {
    Q_OBJECT
public:
    virtual VideoNode *videoNode() = 0;
    virtual QSharedPointer<VideoNode> createCopyForRendering() = 0;
};
