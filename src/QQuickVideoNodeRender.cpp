#include "QQuickVideoNodeRender.h"
#include <QSGImageNode>
#include <QQuickWindow>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

QQuickVideoNodeRender::QQuickVideoNodeRender()
    : m_videoNode(nullptr)
    , m_chain(-1) {
    setFlags(QQuickItem::ItemHasContents);
}

QQuickVideoNodeRender::~QQuickVideoNodeRender() {
}

VideoNode *QQuickVideoNodeRender::videoNode() {
    return m_videoNode.data();
}

void QQuickVideoNodeRender::setVideoNode(VideoNode *videoNode) {
    m_videoNode = QSharedPointer<VideoNode>(videoNode);
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
    qDebug() << "updatePaintNode";
    QSGImageNode *node = static_cast<QSGImageNode *>(oldNode); // TODO non-smart pointer is leaky?
    if (m_chain >= 0 && !m_videoNode.isNull()) {
        oglTexture = m_videoNode->texture(m_chain);
        if (!oglTexture.isNull()) {
            // TODO repeatedly creating the QSGTexture is probably not the most efficient
            sgTexture = QSharedPointer<QSGTexture>(window()->createTextureFromId(oglTexture->textureId(),
                                                                                 QSize(oglTexture->width(), oglTexture->height()),
                                                                                 QQuickWindow::TextureHasAlphaChannel));
            if (!node) {
                node = window()->createImageNode();
                node->setFiltering(QSGTexture::Linear);
                node->setTextureCoordinatesTransform(QSGImageNode::MirrorVertically);
            }

            node->setTexture(sgTexture.data());
            node->markDirty(QSGNode::DirtyForceUpdate);
            if(node) node->setRect(boundingRect());
        }
    }
    return node;
}
