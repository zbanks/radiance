#include "FramebufferVideoNodeRender.h"
#include <QSGImageNode>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

FramebufferVideoNodeRender::FramebufferVideoNodeRender()
    : m_videoNode(nullptr)
    , m_chain(-1)
    , m_size(200, 200)
    , m_blitter() 
    , m_fbo(nullptr) {
    m_blitter.create();
}

FramebufferVideoNodeRender::~FramebufferVideoNodeRender() {

}

VideoNode *FramebufferVideoNodeRender::videoNode() {
    return m_videoNode;
}

void FramebufferVideoNodeRender::setVideoNode(VideoNode *videoNode) {
    m_videoNode = videoNode;
    emit videoNodeChanged(videoNode);
}

int FramebufferVideoNodeRender::chain() {
    return m_chain;
}

void FramebufferVideoNodeRender::setChain(int chain) {
    m_chain = chain;
    emit chainChanged(chain);
}

QImage FramebufferVideoNodeRender::render() {
    auto textureId = m_videoNode->texture(m_chain);
    auto size = m_videoNode->size(m_chain);

    m_fbo = new QOpenGLFramebufferObject(size);
    auto rc = m_fbo->bind();
    m_blitter.bind();

    const QRect targetRect(QPoint(0, 0), size);
    const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, targetRect);
    m_blitter.blit(textureId, target, QOpenGLTextureBlitter::OriginBottomLeft);

    m_blitter.release();
    m_fbo->release();
    return m_fbo->toImage();
}
