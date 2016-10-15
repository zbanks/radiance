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

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

public:
    Effect();
    qreal intensity();
    QString source();
    void setIntensity(qreal value);
    void setSource(QString source);
    Q_INVOKABLE void setPrevious(Effect *effect);

public Q_SLOTS:
    void ready();

signals:
    void intensityChanged(qreal value);
    void sourceChanged(QString value);

private:
    qreal m_intensity;
    QString m_source;
    EffectRenderer *m_renderer;
    Effect *m_previous;
};

#endif
