#pragma once

#include "VideoNode.h"
#include <QObject>

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
    VideoNode *addVideoNode(QString type);
    void removeVideoNode(VideoNode *videoNode);
    void addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);
    void removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput);


signals:
    void videoNodeAdded(VideoNode *videoNode);
    void videoNodeRemoved(VideoNode *videoNode);
    void edgeAdded(Edge edge);
    void edgeRemoved(Edge edge);

private:
    RenderContext *m_context;
    QList<VideoNode *> m_vertices;
    QList<Edge> m_edges;
};
