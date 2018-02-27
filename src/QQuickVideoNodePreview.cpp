#include "QQuickVideoNodePreview.h"
#include <QSGImageNode>
#include <QQuickWindow>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

QQuickVideoNodePreview::QQuickVideoNodePreview()
    : m_videoNodeId(0)
    , m_previewAdapter(nullptr)
    , m_window(nullptr) {
    setFlags(QQuickItem::ItemHasContents);
    connect(this, &QQuickItem::windowChanged, this, &QQuickVideoNodePreview::onWindowChanged);
    onWindowChanged(window());
}

void QQuickVideoNodePreview::onWindowChanged(QQuickWindow *window) {
    if(m_window) disconnect(m_window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
    if(window)   connect(window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
    m_window = window;
}

QQuickVideoNodePreview::~QQuickVideoNodePreview() {
}

int QQuickVideoNodePreview::videoNodeId() {
    return m_videoNodeId;
}

void QQuickVideoNodePreview::setVideoNodeId(int videoNodeId) {
    m_videoNodeId = videoNodeId;
    emit videoNodeIdChanged(videoNodeId);
}

QQuickPreviewAdapter *QQuickVideoNodePreview::previewAdapter() {
    return m_previewAdapter;
}

void QQuickVideoNodePreview::setPreviewAdapter(QQuickPreviewAdapter *previewAdapter) {
    m_previewAdapter = previewAdapter;
    emit previewAdapterChanged(previewAdapter);
}

QSGNode *QQuickVideoNodePreview::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
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

    if (m_previewAdapter && m_videoNodeId) {
        auto textureId = m_previewAdapter->previewTexture(m_videoNodeId);
        if (textureId) {
            // TODO repeatedly creating the QSGTexture is probably not the most efficient
            auto size = m_previewAdapter->previewSize();
            node->setTexture(window()->createTextureFromId(textureId, size, QQuickWindow::TextureHasAlphaChannel));
            node->setRect(boundingRect());
        }
    }
    node->markDirty(QSGNode::DirtyMaterial); // Notifies all connected renderers that the node has dirty bits ;)
    return node;
}
