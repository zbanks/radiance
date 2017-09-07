#pragma once

#include "VideoNode.h"
#include <QObject>
#include <QList>
#include <QVector>
#include <QVariantList>

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
    QVector<VideoNode *> origVertices;
    QVector<QSharedPointer<VideoNode>> vertices;

    // Edges
    QVector<int> fromVertex;
    QVector<int> toVertex;
    QVector<int> toInput;
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
    ModelCopyForRendering createCopyForRendering();

    // This function is called after rendering
    // from the render thread
    // to put the rendered textures and updated states
    // back into the graph.
    // Sometimes nodes are deleted during rendering,
    // these nodes are not updated
    // because they no longer exist.
    void copyBackRenderStates(int chain, QVector<VideoNode *> origVertices, QVector<QSharedPointer<VideoNode>> renderedVertices);

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

signals:
    // Emitted after flush() is called (assuming the graph did actually change)
    // with the interim changes
    void graphChanged(QVariantList verticesAdded, QVariantList verticesRemoved, QVariantList edgesAdded, QVariantList edgesRemoved);

protected:
    void emitGraphChanged();
    QVector<VideoNode *> topoSort();

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
};
