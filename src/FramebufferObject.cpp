#include "FramebufferObject.h"

FramebufferObject::FramebufferObject() {
    glGenFramebuffers(1, &m_fboId);
}

FramebufferObject::~FramebufferObject() {
    glDeleteFramebuffers(1, &m_fboId);
}

void FramebufferObject::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
}

void FramebufferObject::release() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FramebufferObject::setTexture(GLuint textureId) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
}
