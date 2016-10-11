#ifndef __EFFECT_H
#define __EFFECT_H

#include <QtQuick/QQuickFramebufferObject>
#include <QtQuick/QQuickWindow>
#include <QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>

class EffectRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    EffectRenderer();
    ~EffectRenderer();

    void render();
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);

private:
    QOpenGLShaderProgram *m_program;
};

class Effect : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

public:
    Effect();
    qreal intensity();
    QString source();
    void setIntensity(qreal value);
    void setSource(QString source);
    Renderer *createRenderer() const;

signals:
    void intensityChanged(qreal value);
    void sourceChanged(QString value);

private:
    qreal m_intensity;
    QString m_source;
};

#endif
