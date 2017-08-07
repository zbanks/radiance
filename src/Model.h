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

struct ModelGraphEdge {
    int fromVertex;
    int toVertex;
    int toInput;
};

// Return type of graphCopy
struct ModelCopyForRendering {
    QVector<VideoNode *> origVertices;
    QVector<QSharedPointer<VideoNode>> vertices;
    QVector<ModelGraphEdge> edges;
};

// This class is a snapshot of the model
// The vertex list is always in topo-sorted order
class ModelGraph : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<VideoNode> vertices READ qmlVertices)
    Q_PROPERTY(QVariantList edges READ qmlEdges)

public:
    ModelGraph();
   ~ModelGraph() override;
    ModelGraph(QVector<VideoNode *> vertices, QList<Edge> edges);
    ModelGraph(const ModelGraph &other);
    int vertexCount() const;
    VideoNode *vertexAt(int index) const;

    // Careful with this one
    ModelGraph& operator=(const ModelGraph&);

public slots:
    QVector<VideoNode *> vertices() const;
    QVector<ModelGraphEdge> edges() const;
    QQmlListProperty<VideoNode> qmlVertices();
    QVariantList qmlEdges() const;

private:
    QVector<VideoNode *> m_vertices;
    QVector<ModelGraphEdge> m_edges;
    static int vertexCount(QQmlListProperty<VideoNode> *);
    static VideoNode *vertexAt(QQmlListProperty<VideoNode> *, int);
};

class Model : public QObject {
    Q_OBJECT
    Q_PROPERTY(ModelGraph *graph READ qmlGraph NOTIFY qmlGraphChanged)

public:
    Model();
   ~Model() override;
    ModelGraph *qmlGraph();

public slots:
    void addVideoNode(VideoNode *videoNode);
    void removeVideoNode(VideoNode *videoNode);
    void addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    ModelGraph graph();

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

signals:
    void videoNodeAdded(VideoNode *videoNode);
    void videoNodeRemoved(VideoNode *videoNode);
    void edgeAdded(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void edgeRemoved(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void graphChanged(ModelGraph graph);
    void qmlGraphChanged();

protected:
//    Edge *getInputEdge(VideoNode *toVertex, int toInput);
//    Edge *getOutputEdge(VideoNode *fromVertex);
    void regenerateGraph(QVector<VideoNode *> sortedVertices);
    void emitGraphChanged();
    QVector<VideoNode *> topoSort();

private:
    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;
    ModelGraph m_graph;

    // m_vertices and m_edges can be
    // written to by the render thread.
    // This lock ensures that we aren't
    // trying to read or write it
    // from the GUI thread at the same time.
    QMutex m_graphLock;

    // The m_graph can be read by
    // the render thread.
    // This lock ensures that we aren't writing it
    // in the GUI thread
    // while we are trying to generate a copy
    // in the render thread.
    QMutex m_modelGraphLock;
};
