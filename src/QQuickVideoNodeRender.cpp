#include "QQuickVideoNodeRender.h"
#include <QSGImageNode>
#include <QQuickWindow>
#include <QOpenGLTexture>

QQuickVideoNodeRender::QQuickVideoNodeRender()
    : m_videoNode(nullptr)
    , m_chain(-1) {
}

QQuickVideoNodeRender::~QQuickVideoNodeRender() {
}

VideoNode *QQuickVideoNodeRender::videoNode() {
    return m_videoNode;
}

void QQuickVideoNodeRender::setVideoNode(VideoNode *videoNode) {
    m_videoNode = videoNode;
    emit videoNodeChanged(videoNode);
}

int QQuickVideoNodeRender::chain() {
    return m_chain;
}

void QQuickVideoNodeRender::setChain(int chain) {
    m_chain = chain;
    emit chainChanged(chain);
}

QSGNode *QQuickVideoNodeRender::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
    if (!node) {
        node = window()->createImageNode();
    }
    if (m_chain >= 0 && m_videoNode != nullptr) {
        // TODO repeatedly creating the QSGTexture is probably not the most efficient
        QOpenGLTexture *oglTexture = m_videoNode->texture(m_chain);
        QSGTexture *sgTexture = window()->createTextureFromId(oglTexture->textureId(), QSize(oglTexture->width(), oglTexture->height()));
        node->setTexture(sgTexture);
    }
    node->setRect(boundingRect()); // TODO unnecessary?
    return node;
}
