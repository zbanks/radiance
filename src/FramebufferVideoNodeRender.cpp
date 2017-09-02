#include "FramebufferVideoNodeRender.h"
#include <QSGImageNode>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

FramebufferVideoNodeRender::FramebufferVideoNodeRender(QSize size)
    : m_videoNode(nullptr)
    , m_chain(-1)
    , m_size(size)
    , m_blitter()
    , m_fbo(m_size) {
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

    m_fbo.bind();
    m_blitter.bind();

    const QRect targetRect(QPoint(0, 0), m_fbo.size());
    const QRect inputRect(QPoint(0, 0), m_videoNode->size(m_chain));
    // TODO: This doesn't actually work if the two sizes aren't equal :(
    Q_ASSERT(targetRect == inputRect);
    const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, inputRect);
    m_blitter.blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);

    m_blitter.release();
    m_fbo.release();
    return m_fbo.toImage();
}
