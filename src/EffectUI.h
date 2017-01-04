#pragma once

#include "Effect.h"
#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLFramebufferObject>

class EffectUI : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(EffectUI *previous READ previous WRITE setPrevious NOTIFY previousChanged)
    Q_PROPERTY(bool master READ isMaster WRITE setMaster)

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

public:
    EffectUI();
    qreal intensity();
    QString source();
    EffectUI *previous();
    void setIntensity(qreal value);
    void setSource(QString source);
    void setPrevious(EffectUI *source);
    Effect *m_renderer;
    bool isMaster();
    void setMaster(bool master);

signals:
    void intensityChanged(qreal value);
    void sourceChanged(QString value);
    void previousChanged(EffectUI * value);
    void nextFrame();

private:
    EffectUI *m_previous;
    QString m_source;
};
