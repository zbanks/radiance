#include "QQuickVideoNodeRender.h"
#include <QSGImageNode>
#include <QQuickWindow>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include "main.h"

QQuickVideoNodeRender::QQuickVideoNodeRender()
    : m_context(renderContext)
    , m_videoNodeId(0)
    , m_chain(-1)
    , m_window(nullptr) {
    setFlags(QQuickItem::ItemHasContents);
    connect(this, &QQuickItem::windowChanged, this, &QQuickVideoNodeRender::onWindowChanged);
}

void QQuickVideoNodeRender::onWindowChanged(QQuickWindow *window) {
    if(m_window ) disconnect(m_window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
    if(window )   connect(window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
    m_window = window;
}

QQuickVideoNodeRender::~QQuickVideoNodeRender() {
}

VnId QQuickVideoNodeRender::videoNodeId() {
    return m_videoNodeId;
}

void QQuickVideoNodeRender::setVideoNodeId(VnId videoNodeId) {
    m_videoNodeId = videoNodeId;
    emit videoNodeIdChanged(videoNodeId);
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
        node->setFiltering(QSGTexture::Linear);
        node->setTextureCoordinatesTransform(QSGImageNode::MirrorVertically);
        node->setRect(boundingRect());
        node->setOwnsTexture(true);

        // Create a garbage QSGTexture if the real one is not ready yet
        node->setTexture(window()->createTextureFromId(0, QSize(1, 1), QQuickWindow::TextureHasAlphaChannel));
        // It is important that we generate a node even if we are not ready
        // so that we can mark it dirty. If we don't mark it dirty on the first call,
        // this function will never get called again
    }

    //qDebug() << "render with chain" << m_chain << m_videoNode;

    if (m_chain >= 0 && m_videoNodeId != 0) {
        auto textureId = m_context->lastRender(m_chain, m_videoNodeId);
        auto size = m_context->chainSize(m_chain);
        if (textureId != 0) {
            // TODO repeatedly creating the QSGTexture is probably not the most efficient
            node->setTexture(window()->createTextureFromId(textureId, size, QQuickWindow::TextureHasAlphaChannel));
        }
    }
    node->markDirty(QSGNode::DirtyMaterial); // Notifies all connected renderers that the node has dirty bits ;)
    return node;
}
