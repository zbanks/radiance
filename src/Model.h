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
    void ref();
    void deref();

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
    ModelGraph graphRef();

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
    QMutex m_graphLock;
};
