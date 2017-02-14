#pragma once

#include "VideoNode.h"
#include <QQuickFramebufferObject>

class VideoNodeUI : public QQuickFramebufferObject {
    Q_OBJECT

protected:
    QQuickFramebufferObject::Renderer *createRenderer() const;

public:
    VideoNodeUI();
    VideoNode *m_videoNode;
    virtual ~VideoNodeUI();
};
