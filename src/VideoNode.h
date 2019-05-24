#pragma once

#include "Chain.h"
#include "Model.h"
#include <QObject>
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QOpenGLFunctions>
#include <QMutex>

// This is an abstract base class
// for nodes in the DAG.
// VideoNodes have 0 or more inputs, and one output.
// Unless explicitly stated otherwise,
// VideoNode methods are ALL thread-safe.

class Context;

class VideoNode
    : public QObject 
    , public QEnableSharedFromThis<VideoNode>
{
    Q_OBJECT
    Q_PROPERTY(Context *context READ context CONSTANT);
    Q_PROPERTY(int inputCount READ inputCount WRITE setInputCount NOTIFY inputCountChanged);
    Q_PROPERTY(NodeState nodeState READ nodeState WRITE setNodeState NOTIFY nodeStateChanged);

public:
    enum NodeState {
        Ready,
        Loading,
        Broken
    };
    Q_ENUM(NodeState)

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

    // paint() may mutate the RenderState
    // for the chain that it was called on.
    // Returns 0 if the chain does not exist or is not ready.
    virtual GLuint paint(ChainSP chain, QVector<GLuint> inputTextures);

public slots:
    // Convert this VideoNode to JSON
    virtual QJsonObject serialize();

    // Number of inputs
    int inputCount();
    void setInputCount(int value);

    // Context
    Context *context();

    // Chains are context and metadata for a render.
    // Creating and destroying chains may be expensive,
    // so we perform a diff and only tell you
    // which ones that changed through the function chainsEdited.
    QList<ChainSP> chains();
    void setChains(QList<ChainSP> chains);

    // A VideoNode may request of the model
    // that certain chains exist.
    // By default, no chains are requested.
    // You only need to implement this method if you are
    // writing an output. 
    // You must also emit the signals
    // requestedChainAdded and requestedChainRemoved.
    // The Model will add these through setChains.
    virtual QList<ChainSP> requestedChains();

    // The last model that this VideoNode was added to
    // WeakModel encapsulates a weak reference to modeldata
    // because the Model may be deleted out from underneath the VideoNode
    // and we can't give it a strong reference or deadlock would occur
    void setLastModel(QWeakPointer<Model> model);
    QWeakPointer<Model> lastModel();

    // This member variable sets what state the VideoNode is in.
    // This state can be Ready (VideoNode is active and working,)
    // Loading (VideoNode is initializing and will be ready soon,)
    // or Broken (something went wrong and this VideoNode is not functioning as intended.)
    // This is mainly for UI purposes, and the state
    // does not affect how the VideoNode processes video.
    // Even broken VideoNodes should at a minimum
    // pass through video unchanged
    NodeState nodeState();
    void setNodeState(NodeState value);

protected slots:
    // If your node does anything at all, you will need to override this method
    virtual void chainsEdited(QList<ChainSP> added, QList<ChainSP> removed);

signals:
    // Emitted when the object has something to say
    // e.g. due to an error
    void message(QString str);
    void warning(QString str);
    void error(QString str);

    // Emitted when the number of inputs changes
    void inputCountChanged(int value);

    // Emitted when the chains list changes
    void chainsChanged(QList<ChainSP> chains);

    // Emitted when requestedChains changes
    void requestedChainAdded(ChainSP chain);
    void requestedChainRemoved(ChainSP chain);

    // Emitted when state changes
    void nodeStateChanged(NodeState value);

protected:
    VideoNode(Context *context);

    int m_inputCount{};
    QMutex m_stateLock; // TODO this is no longer a meaningful concept, should use separate locks for separate fields
    QList<ChainSP> m_chains;
    Context *m_context{};
    QWeakPointer<Model> m_lastModel;
    VideoNode::NodeState m_nodeState{VideoNode::Ready};
};

typedef QmlSharedPointer<VideoNode> VideoNodeSP;
Q_DECLARE_METATYPE(VideoNodeSP*)
