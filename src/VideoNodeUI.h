#pragma once

#include "VideoNode.h"
#include <QQuickFramebufferObject>

class VideoNodeUI : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(qreal fps READ fps NOTIFY fpsChanged)

protected:
    QQuickFramebufferObject::Renderer *createRenderer() const;

public:
    VideoNodeUI();
    virtual ~VideoNodeUI();

public slots:
    void setFps(qreal value);
    qreal fps();

signals:
    void fpsChanged(qreal value);

private:
    qreal m_fps;
public:
    VideoNode *m_videoNode;
};
