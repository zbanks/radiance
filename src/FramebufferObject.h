#pragma once

#include <QOpenGLFunctions>

// Very simple RAII FBO class

class FramebufferObject {
public:
    FramebufferObject(QOpenGLFunctions *glFuncs);
   ~FramebufferObject();

    void bind();
    void release();
    void setTexture(GLuint textureId);
    GLuint fboId();

private:
    QOpenGLFunctions *m_glFuncs;
    GLuint m_fboId;
    int m_texture;
};
