#pragma once

#include "VideoNode.h"
#include <QQuickItem>

class QQuickVideoNodeRender : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(VideoNode *videoNode READ videoNode WRITE setVideoNode NOTIFY videoNodeChanged)
    Q_PROPERTY(int chain READ chain WRITE setChain NOTIFY chainChanged)

public:
    QQuickVideoNodeRender();
    virtual ~QQuickVideoNodeRender();

    VideoNode *videoNode();
    int chain();
    void setVideoNode(VideoNode *videoNode);
    void setChain(int chain);

signals:
    void videoNodeChanged(VideoNode *videoNode);
    void chainChanged(int chain);

private:
    VideoNode *m_videoNode;
    int m_chain;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
};
