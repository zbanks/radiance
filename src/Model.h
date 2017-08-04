#pragma once

#include "VideoNode.h"
#include <QObject>
#include <QList>

class Model : public QObject {
    Q_OBJECT

public:
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
    void edgeAdded(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void edgeRemoved(VideoNode *fromVertex, VideoNode *toVertex, int toInput);

protected:
//    Edge *getInputEdge(VideoNode *toVertex, int toInput);
//    Edge *getOutputEdge(VideoNode *fromVertex);

private:
    struct Edge {
        VideoNode *fromVertex;
        VideoNode *toVertex;
        int toInput;
    };

    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;
};
