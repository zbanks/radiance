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
class ModelGraph : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<VideoNode> vertices READ qmlVertices)
    Q_PROPERTY(QVariantList edges READ qmlEdges)

public:
    ModelGraph();
    ModelGraph(QList<VideoNode *> vertices, QList<Edge> edges);
    ModelGraph(const ModelGraph &other);
    int vertexCount() const;
    VideoNode *vertexAt(int index) const;

    // Careful with this one
    ModelGraph& operator=(const ModelGraph&);

public slots:
    QVector<VideoNode *> vertices();
    QVector<ModelGraphEdge> edges();
    QQmlListProperty<VideoNode> qmlVertices();
    QVariantList qmlEdges();

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
    void regenerateGraph();
    void emitGraphChanged();

private:
    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;
    ModelGraph m_graph;
};
