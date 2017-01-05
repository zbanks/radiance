#pragma once

#include "VideoNodeUI.h"
#include "Effect.h"
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLFramebufferObject>

class EffectUI : public VideoNodeUI {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(VideoNodeUI *previous READ previous WRITE setPrevious NOTIFY previousChanged)

public:
    EffectUI();
    qreal intensity();
    QString source();
    VideoNodeUI *previous();
    void setIntensity(qreal value);
    void setSource(QString source);
    void setPrevious(VideoNodeUI *source);

signals:
    void intensityChanged(qreal value);
    void sourceChanged(QString value);
    void previousChanged(VideoNodeUI *value);
    void nextFrame();

private:
    void initialize();
    VideoNodeUI *m_previous;
    QString m_source;
};
