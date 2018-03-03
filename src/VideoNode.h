#pragma once

#include "Chain.h"
#include <QObject>
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QOpenGLFunctions>
#include <QMutex>

// This is an abstract base class
// for nodes in the DAG.
// VideoNodes have 0 or more inputs, and one output.
// Unless explicitly stated otherwise,
// VideoNode methods are not thread-safe
// and should be called in the same thread
// or through queued signals and slots.
// The few exceptions to this
// are methods relating to rendering.

class Context;

class VideoNode : public QObject {
    Q_OBJECT
    Q_PROPERTY(Context *context READ context CONSTANT);
    Q_PROPERTY(int inputCount READ inputCount WRITE setInputCount NOTIFY inputCountChanged);
    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged);

public:
    VideoNode(Context *context);

    // VideoNodes must be copyable
    VideoNode(const VideoNode &other);

   ~VideoNode() override;

    // Methods for de/serializing VideoNodes
    virtual QJsonObject serialize();

    // Methods for rendering

    // Creates a copy of this node
    // This function is thread-safe
    // and the ability to run this function from any thread
    // is why any state mutation
    // needs to take the stateLock.
    //
    // This should be a soft copy / propagate changes back to
    // the original.
    //
    // The new object will be assigned to the thread
    // that createCopyForRendering is called in.
    virtual QSharedPointer<VideoNode> createCopyForRendering(Chain chain) = 0;

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
    // Returns 0 if the chain does not exist or is not ready.
    virtual GLuint paint(Chain chain, QVector<GLuint> inputTextures) = 0;

public slots:
    // Number of inputs
    int inputCount();
    void setInputCount(int value);

    // Context
    Context *context();

    // Returns a unique identifier
    // within the context
    // Anything that references this node in the main thread
    // can use its pointer.
    // Anything that references this node outside of the main thread
    // (mostly render artifacts)
    // should use this ID
    // since the pointer may become invalidated or non-unique.
    // This method is thread-safe.
    int id();
    void setId(int id);

    // Chains are context and metadata for a render.
    // Creating and destroying chains may be expensive,
    // so we perform a diff and only tell you
    // which ones that changed through the function chainsEdited.
    QList<Chain> chains();
    void setChains(QList<Chain> chains);

    // A VideoNode may request of the model
    // that certain chains exist.
    // By default, no chains are requested.
    // You only need to implement this method if you are
    // writing an output. 
    // You must also emit the signals
    // requestedChainAdded and requestedChainRemoved.
    // The Model will add these through setChains.
    virtual QList<Chain> requestedChains();

protected slots:
    // If your node does anything at all, you will need to override this method
    // and take appropriate action when the set of render chains changes.
    virtual void chainsEdited(QList<Chain> added, QList<Chain> removed);

protected:
    int m_inputCount{};
    QMutex m_stateLock;
    int m_id;
    QList<Chain> m_chains;
    Context *m_context;

    // This variable stores whether or not this VideoNode is a copy for rendering
    // Some operations are not available on copies.
    bool m_copy{false};

signals:
    // Emitted when the object wishes to be deleted
    // e.g. due to an error
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);

    // Emitted when the number of inputs changes
    void inputCountChanged(int value);

    void chainsChanged(QList<Chain> chains);
    void idChanged(int id);

    // Emitted when requestedChains changes
    void requestedChainAdded(Chain chain);
    void requestedChainRemoved(Chain chain);
};
