#pragma once

#include "VideoNodeOld.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QMutex>

class Effect : public VideoNodeOld {
    Q_OBJECT

public:
    Effect(RenderContext *context);
    ~Effect();
    bool loadProgram(QString name);
    QSet<VideoNodeOld*> dependencies();

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

public slots:
    qreal intensity();
    VideoNodeOld *previous();

    void setIntensity(qreal value);
    void setPrevious(VideoNodeOld *value);

signals:
    void intensityChanged(qreal value);
    void previousChanged(VideoNodeOld *value);

private:
    std::vector<std::shared_ptr<QOpenGLFramebufferObject> > fbos;

    std::vector<std::vector<std::shared_ptr<QOpenGLFramebufferObject>>> m_intermediateFbos;
    std::vector<std::unique_ptr<QOpenGLShaderProgram> > m_programs;
    int m_fboIndex;

    void initialize();
    void paint();

    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_realTime;
    qreal m_realTimeLast;
    VideoNodeOld *m_previous;

    QMutex m_programLock;

    bool m_regenerateFbos;
};

class EffectList : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE static QStringList effectNames();
};

