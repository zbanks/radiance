#pragma once

#include "VideoNode.h"
#include <QObject>
#include <QList>
#include <QVector>
#include <QVariantList>
#include <QQmlListProperty>

struct Edge {
    VideoNode *fromVertex;
    VideoNode *toVertex;
    int toInput;
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
    Q_PROPERTY(QQmlListProperty<VideoNode> vertices READ qmlVertices)
    Q_PROPERTY(QVariantList edges READ qmlEdges)

public:
    Model();
   ~Model() override;

public slots:
    void addVideoNode(VideoNode *videoNode);
    void removeVideoNode(VideoNode *videoNode);
    void addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);

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
    QQmlListProperty<VideoNode> qmlVertices();

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
    void videoNodeAdded(VideoNode *videoNode);
    void videoNodeRemoved(VideoNode *videoNode);
    void edgeAdded(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void edgeRemoved(VideoNode *fromVertex, VideoNode *toVertex, int toInput);

protected:
    void emitGraphChanged();
    QVector<VideoNode *> topoSort();
    int vertexCount() const;
    VideoNode *vertexAt(int index) const;

private:
    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;
    QVector<VideoNode *> m_verticesSortedForRendering;

    // m_vertices and m_edges can be
    // read from or written to by the render thread.
    // This lock ensures that we aren't
    // trying to read or write it
    // from the GUI thread at the same time.
    QMutex m_graphLock;

    static int vertexCount(QQmlListProperty<VideoNode> *);
    static VideoNode *vertexAt(QQmlListProperty<VideoNode> *, int);
};
