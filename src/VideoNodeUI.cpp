#include "VideoNodeUI.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>

class TextureNode : public QObject, public QSGSimpleTextureNode {
    Q_OBJECT

public:
    TextureNode(QQuickWindow *window, VideoNode *videoNode)
        : m_id(0)
        , m_size(0, 0)
        , m_texture(0)
        , m_window(window)
        , m_videoNode(videoNode)
    {
        // Our texture node must have a texture, so use the default 0 texture.
        m_texture = m_window->createTextureFromId(0, QSize(1, 1));
        setTexture(m_texture);
        setFiltering(QSGTexture::Linear);
    }

    ~TextureNode()
    {
        delete m_texture;
    }

signals:
    void pendingNewTexture();

public slots:

    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode() {
        if(m_videoNode->swapPreview()) {
            int newId = m_videoNode->m_displayPreviewFbo->texture();
            QSize size = m_videoNode->m_displayPreviewFbo->size();
            if(m_id != newId || m_size != size) {
                delete m_texture;
                m_texture = m_window->createTextureFromId(newId, size, QQuickWindow::TextureHasAlphaChannel);
                setTexture(m_texture);
                m_id = newId;
                m_size = size;
            }
            markDirty(DirtyMaterial);
        }
    }

private:

    int m_id;
    QSize m_size;

    QMutex m_mutex;

    QSGTexture *m_texture;
    QQuickWindow *m_window;
    VideoNode *m_videoNode;
};

// VideoNodeUI

VideoNodeUI::VideoNodeUI() : m_videoNode(0) {
    setFlag(ItemHasContents, true);
}

bool VideoNodeUI::isMaster() {
    return m_videoNode->isMaster();
}

void VideoNodeUI::setMaster(bool set) {
    m_videoNode->setMaster(set);
}

QSGNode *VideoNodeUI::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    TextureNode *node = static_cast<TextureNode *>(oldNode);

    if (!renderContext->context->shareContext()) {
        QOpenGLContext *current = window()->openglContext();
        // Some GL implementations requres that the currently bound context is
        // made non-current before we set up sharing, so we doneCurrent here
        // and makeCurrent down below while setting up our own context.
        
        renderContext->share(current);
        current->makeCurrent(window());

        //QMetaObject::invokeMethod(this, "ready");
        //return 0;
    }

    if (!node) {
        node = new TextureNode(window(), m_videoNode);
        initialize();

        // When a new texture is ready on the rendering thread, we use a direct connection to
        // the texture node to let it know a new texture can be used. The node will then
        // emit pendingNewTexture which we bind to QQuickWindow::update to schedule a redraw.

        connect(m_videoNode, &VideoNode::textureReady, window(), &QQuickWindow::update, Qt::QueuedConnection);
        connect(window(), &QQuickWindow::beforeRendering, node, &TextureNode::prepareNode, Qt::DirectConnection);
    }

    node->setRect(boundingRect());

    return node;
}

void VideoNodeUI::initialize() {
}

VideoNodeUI::~VideoNodeUI() {
    delete m_videoNode;
    m_videoNode = 0;
}

#include "VideoNodeUI.moc"
