#pragma once

#include "VideoNodeUI.h"
#include "CrossFader.h"
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLFramebufferObject>

class CrossFaderUI : public VideoNodeUI {
    Q_OBJECT
    Q_PROPERTY(qreal parameter READ parameter WRITE setParameter NOTIFY parameterChanged)
    Q_PROPERTY(VideoNodeUI *left READ left WRITE setLeft NOTIFY leftChanged)
    Q_PROPERTY(VideoNodeUI *right READ right WRITE setRight NOTIFY rightChanged)

public:
    CrossFaderUI();
    qreal parameter();
    VideoNodeUI *left();
    VideoNodeUI *right();
    void setParameter(qreal value);
    void setLeft(VideoNodeUI *source);
    void setRight(VideoNodeUI *source);

signals:
    void parameterChanged(qreal value);
    void leftChanged(VideoNodeUI *value);
    void rightChanged(VideoNodeUI *value);

private slots:
    void onInitialized();

private:
    void initialize();
    VideoNodeUI *m_left;
    VideoNodeUI *m_right;
};
