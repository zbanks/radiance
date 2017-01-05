#pragma once

#include "VideoNode.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QMutex>

class CrossFader : public VideoNode {
    Q_OBJECT

public:
    CrossFader(RenderContext *context);
    ~CrossFader();
    bool load();
 
public slots:
    qreal parameter();
    VideoNode *left();
    VideoNode *right();

    void setParameter(qreal value);
    void setLeft(VideoNode *value);
    void setRight(VideoNode *value);

signals:
    void parameterChanged(qreal value);
    void leftChanged(VideoNode *value);
    void rightChanged(VideoNode *value);

private:
    QOpenGLFramebufferObject *m_previewFbo;

    QOpenGLShaderProgram *m_program;
    QOpenGLFramebufferObject *m_blankPreviewFbo;

    void initialize();
    void paint();

    qreal m_parameter;
    VideoNode *m_left;
    VideoNode *m_right;

    QMutex m_parameterLock;
    QMutex m_leftLock;
    QMutex m_rightLock;
    QMutex m_programLock;
};
