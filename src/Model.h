#pragma once

#include "VideoNode.h"
#include <QObject>
#include <QDebug>
#include <QList>
#include <QVector>
#include <QVariantList>
#include <QJsonObject>

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
    QVector<QSharedPointer<VideoNode>> vertices;

    // Edges, as indices into vertices
    QVector<int> fromVertex;
    QVector<int> toVertex;
    QVector<int> toInput;

    // Render this model
    // The return value is a mapping of VideoNode IDs to OpenGL textures
    // that were rendered into
    QMap<int, GLuint> render(QSharedPointer<Chain> chain);
};

class Model : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList vertices READ qmlVertices)
    Q_PROPERTY(QVariantList edges READ qmlEdges)

public:
    Model();
   ~Model() override;

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
    ModelCopyForRendering createCopyForRendering(QSharedPointer<Chain> chain);

    // Returns a list of vertices
    // in the order they were added
    QList<VideoNode *> vertices() const;

    // Returns a list of edges
    // in the order they were added
    QList<Edge> edges() const;

    // Returns a list of vertices
    // in the order they were added
    // suitable for QML / Javascript
    QVariantList qmlVertices() const;

    // Returns a list of edges
    // in the order they were added
    // suitable for QML / Javascript
    QVariantList qmlEdges() const;

    // Returns a list of verticies that
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
    QList<QSharedPointer<Chain>> chains();
    void addChain(QSharedPointer<Chain> chain);
    void removeChain(QSharedPointer<Chain> chain);

    QJsonObject serialize();
    void deserialize(Context *context, Registry *registry, const QJsonObject &data);

    // These are to wrap serialize/deserialize for the UI
    // TODO: This almost certainly should live elsewhere.
    void loadFile(Context *context, Registry *registry, QString filename);
    void saveFile(QString filename);

signals:
    // Emitted after flush() is called (assuming the graph did actually change)
    // with the interim changes
    void graphChanged(QVariantList verticesAdded, QVariantList verticesRemoved, QVariantList edgesAdded, QVariantList edgesRemoved);

    void chainsChanged(QList<QSharedPointer<Chain>> chains);

    void message(VideoNode *videoNode, QString str);
    void warning(VideoNode *videoNode, QString str);
    void fatal(VideoNode *videoNode, QString str);

protected:
    void emitGraphChanged();
    QVector<VideoNode *> topoSort();
    void prepareNode(VideoNode * node);
    void disownNode(VideoNode * node);

protected slots:
    void onMessage(QString message);
    void onWarning(QString str);
    void onFatal(QString str);

private:
    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;
    QList<VideoNode *> m_verticesForRendering;
    QList<Edge> m_edgesForRendering;
    QVector<VideoNode *> m_verticesSortedForRendering;

    // m_vertices and m_edges can be
    // read from or written to by the render thread.
    // This lock ensures that we aren't
    // trying to read or write it
    // from the GUI thread at the same time.
    QMutex m_graphLock;

    // Chains used for rendering this model
    QList<QSharedPointer<Chain>> m_chains;

    // Counter to give VideoNodes unique IDs
    int m_vnId;
};
