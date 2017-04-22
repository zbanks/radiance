#pragma once

#include "VideoNode.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QMutex>

class Effect : public VideoNode {
    Q_OBJECT

public:
    Effect(RenderContext *context);
    ~Effect();
    bool loadProgram(QString name);
    QSet<VideoNode*> dependencies();

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

public slots:
    qreal intensity();
    VideoNode *previous();

    void setIntensity(qreal value);
    void setPrevious(VideoNode *value);

signals:
    void intensityChanged(qreal value);
    void previousChanged(VideoNode *value);

private:
    QVector<QOpenGLFramebufferObject *> fbos;

    QVector<QVector<QOpenGLFramebufferObject *>> m_intermediateFbos;
    QVector<QOpenGLShaderProgram *> m_programs;
    QVector<QOpenGLFramebufferObject *> m_blankFbos;
    int m_fboIndex;

    void initialize();
    void paint();

    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_realTime;
    qreal m_realTimeLast;
    VideoNode *m_previous;

    QMutex m_intensityLock;
    QMutex m_programLock;
    QMutex m_previousLock;

    bool m_regenerateFbos;
};

class EffectList : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE static QStringList effectNames();
};
