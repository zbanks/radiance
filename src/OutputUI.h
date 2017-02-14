#pragma once

#include "VideoNodeUI.h"
#include <QQuickFramebufferObject>

class OutputWindow;

class OutputUI : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(VideoNodeUI *source READ source WRITE setSource NOTIFY sourceChanged)

protected:
    OutputWindow *m_outputWindow;

public:
    VideoNodeUI *source();
    void setSource(VideoNodeUI *);
    OutputUI();
    VideoNodeUI *m_source;
    QMutex m_sourceLock;

public slots:
    void show();
    void hide();

signals:
    void sourceChanged(VideoNodeUI *value);
};
