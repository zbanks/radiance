#include "FramebufferVideoNodeRender.h"
#include <QSGImageNode>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

FramebufferVideoNodeRender::FramebufferVideoNodeRender(QSize size)
    : m_fbo(size)
    , m_blitter() {
    m_blitter.create();
}

FramebufferVideoNodeRender::~FramebufferVideoNodeRender() {
}

QImage FramebufferVideoNodeRender::render(GLuint textureId) {
    m_fbo.bind();
    m_blitter.bind();

    const QRect targetRect(QPoint(0, 0), m_fbo.size());
    // TODO: This assumes the two sizes are equal
    const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, targetRect);
    m_blitter.blit(textureId, target, QOpenGLTextureBlitter::OriginBottomLeft);

    m_blitter.release();
    m_fbo.release();
    return m_fbo.toImage();
}
