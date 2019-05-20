#pragma once

#include "QmlSharedPointer.h"
#include <QObject>
#include <QDebug>
#include <QList>
#include <QVector>
#include <QVariantList>
#include <QJsonObject>
#include <QOpenGLFunctions>
#include <QMutex>

#include "Chain.h"

class VideoNode;
//class Chain;
//typedef QmlSharedPointer<Chain> ChainSP;
class Registry;
class Context;

struct Edge {
    VideoNode *fromVertex;
    VideoNode *toVertex;
    int toInput;

public:
    QVariantMap toVariantMap() const;
    bool operator==(const Edge &other) const;
};

// Return type of graphCopy
struct ModelCopyForRendering {
    // Copies of the vertices
    // (The QSP here is just to ensure they get deleted)
    QVector<QSharedPointer<VideoNode>> vertices;

    // Edges, as indices into vertices
    QVector<int> fromVertex;
    QVector<int> toVertex;
    QVector<int> toInput;

    // Render this model
    // The return value is a mapping of VideoNodes to OpenGL textures
    // that were rendered into
    QMap<VideoNode, GLuint> render(ChainSP chain);
};

// These functions are not thread-safe unless noted.

class Model
    : public QObject
    , public QEnableSharedFromThis<Model>
{
    Q_OBJECT
    Q_PROPERTY(QVariantList vertices READ qmlVertices)
    Q_PROPERTY(QVariantList edges READ qmlEdges)

public:
    Model();

    static ModelCopyForRendering createCopyForRendering(QWeakPointer<Model> model);

    // Returns a list of vertices
    // in the order they were added
    QList<VideoNode *> vertices() const;

    // Returns a list of edges
    // in the order they were added
    QList<Edge> edges() const;

public slots:
    // These functions mutate the graph.
    // Calling these functions does not emit signals
    // or change what is rendered
    // until flush() is called.
    void addVideoNode(VideoNode *videoNode);
    void removeVideoNode(VideoNode *videoNode);
    void addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);

    // Delete all nodes & edges (still need to call flush())
    void clear();

    // Atomically update the graph used for rendering
    // and emit signals describing how the graph was changed.
    // Call this after adding or removing nodes or edges.
    void flush();

    // This function is called before rendering
    // from the render thread
    // to create a temporary copy of the VideoNodes
    // and their connections.
    // This copy is necessary because
    // sometimes nodes are deleted or edited during rendering.
    // This function is thread-safe
    ModelCopyForRendering createCopyForRendering();

    // Returns a list of vertices
    // in the order they were added
    // suitable for QML / Javascript
    QVariantList qmlVertices() const;

    // Returns a list of edges
    // in the order they were added
    // suitable for QML / Javascript
    QVariantList qmlEdges() const;

    // Returns a list of vertices that
    // are ancestors of the given node
    QList<VideoNode *> ancestors(VideoNode *node);

    // Returns true if `parent`
    // is an ancestor of `child`
    bool isAncestor(VideoNode *parent, VideoNode *child);

    // Chains are instances of the
    // model render pipeline
    // You need a different chain for a different size / shape output,
    // or a different thead.
    // When requesting a render of the model,
    // you must use one of its chains.
    QList<ChainSP> chains();
    void addChain(ChainSP chain);
    void removeChain(ChainSP chain);

    QJsonObject serialize();
    void deserialize(Context *context, Registry *registry, const QJsonObject &data);

    // These are to wrap serialize/deserialize for the UI
    void load(Context *context, Registry *registry, QString filename);
    void save(QString filename);
    void loadDefault(Context *context, Registry *registry);
    void saveDefault();

signals:
    // Emitted after flush() is called (assuming the graph did actually change)
    // with the interim changes
    void graphChanged(QVariantList verticesAdded, QVariantList verticesRemoved, QVariantList edgesAdded, QVariantList edgesRemoved);

    void chainsChanged(QList<ChainSP> chains);

    void message(VideoNode *videoNode, QString str);
    void warning(VideoNode *videoNode, QString str);
    void error(VideoNode *videoNode, QString str);

protected:
    void emitGraphChanged();
    QVector<VideoNode *> topoSort();
    void prepareNode(VideoNode * node);
    void disownNode(VideoNode * node);

    // m_vertices and m_edges must not be accessed from
    // other threads.
    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;

    // m_verticesForRendering, m_edgesForRendering
    // and m_verticesSortedForRendering
    // may be accessed from other threads
    // as long as the m_graphLock is taken.
    QList<VideoNode *> m_verticesForRendering;
    QList<Edge> m_edgesForRendering;
    QVector<VideoNode *> m_verticesSortedForRendering;

    // m_verticesForRendering, m_edgesForRendering
    // and m_verticesSortedForRendering
    // can be read from the render thread.
    // This lock ensures that we aren't
    // trying to write it
    // from the GUI thread at the same time.
    QMutex m_graphLock;

    // Chains used for rendering this model
    QList<ChainSP> m_chains;

protected slots:
    void onMessage(QString message);
    void onWarning(QString str);
    void onError(QString str);
};

typedef QmlSharedPointer<Model> ModelSP;
Q_DECLARE_METATYPE(ModelSP*)
