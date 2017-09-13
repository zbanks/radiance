#pragma once

#include "VideoNode.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLTextureBlitter>

class FramebufferVideoNodeRender : public QObject {
    Q_OBJECT

public:
    FramebufferVideoNodeRender(QSize size = QSize(200, 200));
    virtual ~FramebufferVideoNodeRender();
    QImage render(GLuint textureId);

private:
    QOpenGLFramebufferObject m_fbo;
    QOpenGLTextureBlitter m_blitter;
};
