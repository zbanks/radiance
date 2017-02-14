#pragma once

#include "VideoNodeUI.h"
#include <QQuickFramebufferObject>

class OutputUI : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(VideoNodeUI *source READ source WRITE setSource NOTIFY sourceChanged)

protected:
    QQuickFramebufferObject::Renderer *createRenderer() const override;

public:
    VideoNodeUI *source();
    void setSource(VideoNodeUI *);
    OutputUI();
    VideoNodeUI *m_source;
    QMutex m_sourceLock;

signals:
    void sourceChanged(VideoNodeUI *value);
};
