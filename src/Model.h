#pragma once

#include "VideoNodeOld.h"
#include <QObject>
#include <QSharedPointer>

class Model : public QObject {
    Q_OBJECT

public:
    struct Edge {
        QSharedPointer<VideoNode> fromVertex;
        QSharedPointer<VideoNode> toVertex;
        int toInput;
    };

    Model();
   ~Model() override;
    RenderContext *renderContext();

public slots:
    QSharedPointer<VideoNode> addVideoNode(QString type);
    void removeVideoNode(QSharedPointer<VideoNode> videoNode);
    void addEdge(QSharedPointer<VideoNode> fromVertex, QSharedPointer<VideoNode> toVertex, int toInput);
    void removeEdge(QSharedPointer<VideoNode> fromVertex, QSharedPointer<VideoNode> toVertex, int toInput);

signals:
    void videoNodeAdded(QSharedPointer<VideoNode> videoNode);
    void videoNodeRemoved(QSharedPointer<VideoNode> videoNode);
    void edgeAdded(Edge edge);
    void edgeRemoved(Edge edge);

protected:
    Edge *getInputEdge(QSharedPointer<VideoNode> toVertex, int toInput);
    Edge *getOutputEdge(QSharedPointer<VideoNode> fromVertex);

private:
    RenderContext *m_context;
    QList<QSharedPointer<VideoNode> > m_vertices;
    QList<Edge> m_edges;
};
