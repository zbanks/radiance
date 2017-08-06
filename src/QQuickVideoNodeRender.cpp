#include "QQuickVideoNodeRender.h"
#include <QSGImageNode>
#include <QQuickWindow>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

QQuickVideoNodeRender::QQuickVideoNodeRender()
    : m_videoNode(nullptr)
    , m_chain(-1)
    , m_window(nullptr) {
    setFlags(QQuickItem::ItemHasContents);
    connect(this, &QQuickItem::windowChanged, this, &QQuickVideoNodeRender::onWindowChanged);
}

void QQuickVideoNodeRender::onWindowChanged(QQuickWindow *window) {
    if(m_window != nullptr) disconnect(m_window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
    if(window != nullptr)   connect(window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
    m_window = window;
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
    QSGImageNode *node = static_cast<QSGImageNode *>(oldNode); // TODO non-smart pointer is leaky?
    if (m_chain >= 0 && !m_videoNode.isNull()) {
        oglTexture = m_videoNode->texture(m_chain);
        if (!oglTexture.isNull()) {
            // TODO repeatedly creating the QSGTexture is probably not the most efficient
            sgTexture = QSharedPointer<QSGTexture>(window()->createTextureFromId(oglTexture->textureId(),
                                                                                 QSize(oglTexture->width(), oglTexture->height()),
                                                                                 QQuickWindow::TextureHasAlphaChannel));
        } else {
            // Create a garbage QSGTexture if the real one is not ready yet
            sgTexture = QSharedPointer<QSGTexture>(window()->createTextureFromId(0, QSize(1, 1), QQuickWindow::TextureHasAlphaChannel));
            // It is important that we generate a node even if we are not ready
            // so that we can mark it dirty. If we don't mark it dirty on the first call,
            // this function will never get called again
        }
        if (!node) {
            node = window()->createImageNode();
            node->setFiltering(QSGTexture::Linear);
            node->setTextureCoordinatesTransform(QSGImageNode::MirrorVertically);
            node->markDirty(QSGNode::DirtyForceUpdate); // Notifies all connected renderers that the node has dirty bits ;)
        }

        node->setTexture(sgTexture.data());
        if(node) node->setRect(boundingRect());
    }
    return node;
}
