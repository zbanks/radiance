#include "QQuickVideoNodeRender.h"
#include <QSGImageNode>
#include <QQuickWindow>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include "main.h"

QQuickVideoNodeRender::QQuickVideoNodeRender()
    : m_videoNodeId(0)
    , m_context(nullptr)
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

int QQuickVideoNodeRender::videoNodeId() {
    return m_videoNodeId;
}

void QQuickVideoNodeRender::setVideoNodeId(int videoNodeId) {
    m_videoNodeId = videoNodeId;
    emit videoNodeIdChanged(videoNodeId);
}

Context *QQuickVideoNodeRender::context() {
    return m_context;
}

void QQuickVideoNodeRender::setContext(Context *context) {
    m_context = context;
    emit contextChanged(context);
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

    if (m_context != nullptr && m_videoNodeId != 0) {
        auto textureId = m_context->previewTexture(m_videoNodeId);
        auto size = m_context->previewSize();
        if (textureId != 0) {
            qDebug() << this << "showing texture" << textureId;
            // TODO repeatedly creating the QSGTexture is probably not the most efficient
            node->setTexture(window()->createTextureFromId(textureId, size, QQuickWindow::TextureHasAlphaChannel));
            node->setRect(boundingRect());
        }
    }
    node->markDirty(QSGNode::DirtyMaterial); // Notifies all connected renderers that the node has dirty bits ;)
    return node;
}
