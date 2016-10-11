#ifndef __EFFECT_H
#define __EFFECT_H

#include <QtQuick/QQuickItem>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>

class EffectRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    EffectRenderer();
    ~EffectRenderer();

    void setViewportSize(const QSize &size) { m_viewportSize = size; }
    void setWindow(QQuickWindow *window) { m_window = window; }

public slots:
    void paint();

private:
    QSize m_viewportSize;
    QOpenGLShaderProgram *m_program;
    QQuickWindow *m_window;
};

class Effect : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

public:
    Effect();
    qreal intensity();
    QString source();
    void setIntensity(qreal value);
    void setSource(QString source);

public slots:
    void sync();
    void cleanup();

private slots:
    void onWindowChanged(QQuickWindow *win);

signals:
    void intensityChanged(qreal value);
    void sourceChanged(QString value);

private:
    qreal m_intensity;
    QString m_source;
    EffectRenderer *m_renderer;
};

#endif
