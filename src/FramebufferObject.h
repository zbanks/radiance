#pragma once

#include <QOpenGLFunctions>

// Very simple RAII FBO class

class FramebufferObject : private QOpenGLFunctions {
public:
    FramebufferObject();
   ~FramebufferObject();

    void bind();
    void release();
    void setTexture(GLuint textureId);

private:
    GLuint m_fboId;
    int m_texture;
};
