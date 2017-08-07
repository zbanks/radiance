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
        node->setFiltering(QSGTexture::Linear);
        node->setTextureCoordinatesTransform(QSGImageNode::MirrorVertically);
        node->setRect(boundingRect());

        // Create a garbage QSGTexture if the real one is not ready yet
        sgTexture = QSharedPointer<QSGTexture>(window()->createTextureFromId(0, QSize(1, 1), QQuickWindow::TextureHasAlphaChannel));
        node->setTexture(sgTexture.data());
        // It is important that we generate a node even if we are not ready
        // so that we can mark it dirty. If we don't mark it dirty on the first call,
        // this function will never get called again
    }

    //qDebug() << "render with chain" << m_chain << m_videoNode;

    if (m_chain >= 0 && m_videoNode != nullptr) {
        auto textureId = m_videoNode->texture(m_chain);
        auto size = m_videoNode->size(m_chain);
        if (textureId != 0) {
            // TODO repeatedly creating the QSGTexture is probably not the most efficient
            sgTexture = QSharedPointer<QSGTexture>(window()->createTextureFromId(textureId, size, QQuickWindow::TextureHasAlphaChannel));
            node->setTexture(sgTexture.data());
        }
    }
    node->markDirty(QSGNode::DirtyMaterial); // Notifies all connected renderers that the node has dirty bits ;)
    return node;
}
