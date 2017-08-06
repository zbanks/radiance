#pragma once

#include <QOpenGLFunctions>

// Very simple RAII FBO class

class FramebufferObject : private QOpenGLFunctions {
public:
    FramebufferObject(QOpenGLFunctions *glFuncs);
   ~FramebufferObject();

    void bind();
    void release();
    void setTexture(GLuint textureId);

private:
    QOpenGLFunctions *m_glFuncs;
    GLuint m_fboId;
    int m_texture;
};
