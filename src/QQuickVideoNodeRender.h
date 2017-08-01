#pragma once

#include "VideoNodeOld.h"
#include <QQuickItem>

class QQuickVideoNodeRender : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(VideoNodeOld *videoNode READ videoNode WRITE setVideoNodeOld NOTIFY videoNodeChanged)
    Q_PROPERTY(int chain READ chain WRITE setChain NOTIFY chainChanged)

public:
    QQuickVideoNodeRender();
    virtual ~QQuickVideoNodeRender();

    VideoNodeOld *videoNode();
    int chain();
    void setVideoNodeOld(VideoNodeOld *videoNode);
    void setChain(int chain);

signals:
    void videoNodeChanged(VideoNodeOld *videoNode);
    void chainChanged(int chain);

private:
    VideoNodeOld *m_videoNode;
    int m_chain;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
};
