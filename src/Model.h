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
typedef QmlSharedPointer<VideoNode> VideoNodeSP;

class Registry;
class Context;

struct Edge {
    VideoNodeSP *fromVertex;
    VideoNodeSP *toVertex;
    int toInput;

public:
    QVariantMap toVariantMap() const;
    bool operator==(const Edge &other) const;
};

// Return type of graphCopy
struct ModelCopyForRendering {
    // Copies of the vertices
    QVector<QSharedPointer<VideoNode>> vertices;

    // Edges, as indices into vertices
    QVector<int> fromVertex;
    QVector<int> toVertex;
    QVector<int> toInput;

    // Render this model
    // The return value is a mapping of VideoNodes to OpenGL textures
    // that were rendered into
    QMap<QSharedPointer<VideoNode>, GLuint> render(QSharedPointer<Chain> chain);
};

// These functions are not thread-safe unless noted.

class Model
    : public QObject
    , public QEnableSharedFromThis<QObject>
{
    Q_OBJECT
    Q_PROPERTY(QVariantList vertices READ qmlVertices)
    Q_PROPERTY(QVariantList edges READ qmlEdges)

public:
    Model();

    static ModelCopyForRendering createCopyForRendering(QWeakPointer<Model> model);

    // Returns a list of vertices
    // in the order they were added
    QList<VideoNodeSP *> vertices() const;

    // Returns a list of edges
    // in the order they were added
    QList<Edge> edges() const;

public slots:
    // These functions mutate the graph.
    // Calling these functions does not emit signals
    // or change what is rendered
    // until flush() is called.
    void addVideoNode(VideoNodeSP *videoNode);
    void removeVideoNode(VideoNodeSP *videoNode);
    void addEdge(VideoNodeSP *fromVertex, VideoNodeSP *toVertex, int toInput);
    void removeEdge(VideoNodeSP *fromVertex, VideoNodeSP *toVertex, int toInput);

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
    QList<VideoNodeSP *> ancestors(VideoNodeSP *node) const;

    // Returns true if `parent`
    // is an ancestor of `child`
    bool isAncestor(VideoNodeSP *parent, VideoNodeSP *child) const;

    // Returns a list of vertices that
    // are ancestors of the given node
    // suitable for QML / Javascript
    QVariantList qmlAncestors(VideoNodeSP *node) const;

    // Returns a list of vertices, that
    // are descendants of the given node
    QList<VideoNodeSP *> descendants(VideoNodeSP *node) const;

    // Returns true if `parent`
    // is an descendant of `child`
    bool isDescendant(VideoNodeSP *parent, VideoNodeSP *child) const;

    // Returns a list of vertices that
    // are ancestors of the given node
    // suitable for QML / Javascript
    QVariantList qmlDescendants(VideoNodeSP *node) const;

    // Chains are instances of the
    // model render pipeline
    // You need a different chain for a different size / shape output,
    // or a different thead.
    // When requesting a render of the model,
    // you must use one of its chains.
    QList<QSharedPointer<Chain>> chains();
    void addChain(QSharedPointer<Chain> chain);
    void removeChain(QSharedPointer<Chain> chain);

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

    void chainsChanged(QList<QSharedPointer<Chain>> chains);

    void message(VideoNodeSP *videoNode, QString str);
    void warning(VideoNodeSP *videoNode, QString str);
    void error(VideoNodeSP *videoNode, QString str);

protected:
    void emitGraphChanged();
    QVector<QSharedPointer<VideoNode>> topoSort();
    void prepareNode(VideoNodeSP *node);
    void disownNode(VideoNodeSP *node);

    // m_vertices and m_edges must not be accessed from
    // other threads.
    QList<VideoNodeSP *> m_vertices;
    QList<Edge> m_edges;

    // m_verticesForRendering, m_edgesForRendering
    // and m_verticesSortedForRendering
    // may be accessed from other threads
    // as long as the m_graphLock is taken.
    QList<VideoNodeSP *> m_verticesForRendering;
    QList<Edge> m_edgesForRendering;
    QVector<QSharedPointer<VideoNode>> m_verticesSortedForRendering;

    // m_verticesForRendering, m_edgesForRendering
    // and m_verticesSortedForRendering
    // can be read from the render thread.
    // This lock ensures that we aren't
    // trying to write it
    // from the GUI thread at the same time.
    QMutex m_graphLock;

    // Chains used for rendering this model
    QList<QSharedPointer<Chain>> m_chains;

    // Find which VideoNodeSP* in this model
    // emitted a signal
    VideoNodeSP *lookupSender();

protected slots:
    void onMessage(QString message);
    void onWarning(QString str);
    void onError(QString str);
};

typedef QmlSharedPointer<Model> ModelSP;
Q_DECLARE_METATYPE(ModelSP*)
