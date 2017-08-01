#pragma once

#include "VideoNodeOld.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QMutex>

class CrossFader : public VideoNodeOld {
    Q_OBJECT

public:
    CrossFader(RenderContextOld *context);
    ~CrossFader();
    bool load();
    QSet<VideoNodeOld*> dependencies();

public slots:
    qreal parameter();
    VideoNodeOld *left();
    VideoNodeOld *right();

    void setParameter(qreal value);
    void setLeft(VideoNodeOld *value);
    void setRight(VideoNodeOld *value);

    std::shared_ptr<QOpenGLFramebufferObject> & blankFbo(int i);
    std::shared_ptr<QOpenGLFramebufferObject> const & blankFbo(int i) const;
signals:
    void parameterChanged(qreal value);
    void leftChanged(VideoNodeOld *value);
    void rightChanged(VideoNodeOld *value);

private:
    std::shared_ptr<QOpenGLShaderProgram> m_program;
    std::vector<std::shared_ptr<QOpenGLFramebufferObject> > m_blankFbos;

    void initialize();
    void paint();

    qreal m_parameter;
    VideoNodeOld *m_left;
    VideoNodeOld *m_right;

    QMutex m_parameterLock;
    QMutex m_leftLock;
    QMutex m_rightLock;
    QMutex m_programLock;
};
