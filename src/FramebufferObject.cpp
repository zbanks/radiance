#include "FramebufferObject.h"

FramebufferObject::FramebufferObject(QOpenGLFunctions *glFuncs)
    : m_glFuncs(glFuncs) {
    m_glFuncs->glGenFramebuffers(1, &m_fboId);
}

FramebufferObject::~FramebufferObject() {
    m_glFuncs->glDeleteFramebuffers(1, &m_fboId);
}

void FramebufferObject::bind() {
    m_glFuncs->glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
}

void FramebufferObject::release() {
    m_glFuncs->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FramebufferObject::setTexture(GLuint textureId) {
    m_glFuncs->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
}
