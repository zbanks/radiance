#pragma once

#include "VideoNode.h"
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLFramebufferObject>

class VideoNodeUI : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool master READ isMaster WRITE setMaster)

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    virtual void initialize();

public:
    VideoNodeUI();
    VideoNode *m_videoNode;
    bool isMaster();
    void setMaster(bool master);
    virtual ~VideoNodeUI();
};
