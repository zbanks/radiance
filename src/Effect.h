#ifndef __EFFECT_H
#define __EFFECT_H

#include <QtQuick/QQuickFramebufferObject>
#include <QtQuick/QQuickWindow>
#include <QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>

class EffectRenderer;

class Effect : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Effect *previous READ previous WRITE setPrevious NOTIFY previousChanged)

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

public:
    Effect();
    qreal intensity();
    QString source();
    Effect *previous();
    void setIntensity(qreal value);
    void setSource(QString source);
    void setPrevious(Effect *source);
    QOpenGLFramebufferObject *previewFbo;
    EffectRenderer *m_renderer;

public Q_SLOTS:
    void ready();
    void nextFrame();

signals:
    void intensityChanged(qreal value);
    void sourceChanged(QString value);
    void previousChanged(Effect * value);
    void renderFinished();

private:
    qreal m_intensity;
    QString m_source;
    Effect *m_previous;
};

#endif
