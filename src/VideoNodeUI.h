#pragma once

#include "VideoNode.h"
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLFramebufferObject>

class VideoNodeUI : public QQuickItem {
    Q_OBJECT

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    virtual void initialize();

public:
    VideoNodeUI();
    VideoNode *m_videoNode;
    virtual ~VideoNodeUI();
};
