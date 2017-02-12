#pragma once

#include "VideoNodeUI.h"
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLFramebufferObject>

class OutputUI : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(VideoNodeUI *source READ source WRITE setSource NOTIFY sourceChanged)

public:
    VideoNodeUI *source();
    void setSource(VideoNodeUI *);
    OutputUI();
    VideoNodeUI *m_source;
    QMutex m_sourceLock;

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

signals:
    void sourceChanged(VideoNodeUI *value);
};
