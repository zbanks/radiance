#pragma once

#include "VideoNodeUI.h"
#include "Effect.h"
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLFramebufferObject>

class EffectUI : public VideoNodeUI {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensityInvoker WRITE setIntensityInvoker NOTIFY intensityChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(VideoNodeUI *previous READ previous WRITE setPrevious NOTIFY previousChanged)

public:
    EffectUI(QString source = QString());
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
    void setIntensityInvoker(qreal value);
    void setPreviousInvoker(VideoNodeOld *source);
    qreal intensityInvoker();
    qreal fpsInvoker();
    void fpsChanged(qreal value);

private:
    void initialize();
    VideoNodeUI *m_previous;
    QString m_source;
};
