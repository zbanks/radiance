#pragma once

#include "VideoNode.h"
#include <QObject>
#include <QList>

class Model : public QObject {
    Q_OBJECT

public:
    struct Edge {
        VideoNode *fromVertex;
        VideoNode *toVertex;
        int toInput;
    };

    Model();
   ~Model() override;

public slots:
    void addVideoNode(VideoNode *videoNode);
    void removeVideoNode(VideoNode *videoNode);
    void addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);

signals:
    void videoNodeAdded(VideoNode *videoNode);
    void videoNodeRemoved(VideoNode *videoNode);
    void edgeAdded(Edge edge);
    void edgeRemoved(Edge edge);

protected:
    Edge *getInputEdge(VideoNode *toVertex, int toInput);
    Edge *getOutputEdge(VideoNode *fromVertex);

private:
    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;
};
