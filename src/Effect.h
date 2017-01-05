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
 
public slots:
    qreal intensity();
    VideoNode *previous();

    void setIntensity(qreal value);
    void setPrevious(VideoNode *value);

signals:
    void intensityChanged(qreal value);
    void previousChanged(VideoNode *value);

private:
    QOpenGLFramebufferObject *previewFbo;

    QVector<QOpenGLFramebufferObject *> m_previewFbos;
    QVector<QOpenGLShaderProgram *> m_programs;
    QOpenGLFramebufferObject *m_blankPreviewFbo;
    int m_fboIndex;

    void initialize();
    void paint();

    qreal m_intensity;
    VideoNode *m_previous;

    QMutex m_intensityLock;
    QMutex m_programLock;
    QMutex m_previousLock;

    bool m_regeneratePreviewFbos;
};
