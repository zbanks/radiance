#include "OutputUI.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>

class TextureNode2 : public QObject, public QSGSimpleTextureNode {
    Q_OBJECT

public:
    TextureNode2(QQuickWindow *window, OutputUI *output)
        : m_id(0)
        , m_size(0, 0)
        , m_texture(0)
        , m_window(window)
        , m_output(output)
    {
        // Our texture node must have a texture, so use the default 0 texture.
        m_texture = m_window->createTextureFromId(0, QSize(1, 1));
        setTexture(m_texture);
        //QSGMaterial *m = material();
        //m->setFlag(QSGMaterial::Blending);
        //setMaterial(m);
        setFiltering(QSGTexture::Linear);
    }

    ~TextureNode2()
    {
        delete m_texture;
    }

signals:
    void pendingNewTexture();

public slots:

    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode() {
        QMutexLocker locker(&m_output->m_sourceLock);
        if(m_output->m_source == NULL) return;
        auto videoNode = m_output->m_source->m_videoNode;
        if(videoNode->swap(videoNode->context()->outputFboIndex())) {
            auto newId = videoNode->m_displayFbos.at(videoNode->context()->outputFboIndex())->texture();
            auto size = videoNode->m_displayFbos.at(videoNode->context()->outputFboIndex())->size();
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
    OutputUI *m_output;
};

// OutputUI

OutputUI::OutputUI() : m_source(0) {
    setFlag(ItemHasContents, true);
}

QSGNode *OutputUI::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    TextureNode2 *node = static_cast<TextureNode2 *>(oldNode);

    if (!node) {
        node = new TextureNode2(window(), this);

        // When a new texture is ready on the rendering thread, we use a direct connection to
        // the texture node to let it know a new texture can be used. The node will then
        // emit pendingNewTexture which we bind to QQuickWindow::update to schedule a redraw.

        connect(window(), &QQuickWindow::beforeRendering, node, &TextureNode2::prepareNode, Qt::DirectConnection);
    }

    node->setRect(boundingRect());

    return node;
}

VideoNodeUI *OutputUI::source() {
    QMutexLocker locker(&m_sourceLock);
    return m_source;
}

void OutputUI::setSource(VideoNodeUI *value) {
    {
        QMutexLocker locker(&m_sourceLock);
        if(m_source == value) return;
        if(m_source != NULL) {
        //    disconnect(m_source->m_videoNode, &VideoNode::textureReady, window(), &QQuickWindow::update);
        }
        m_source = value;
        //connect(m_source->m_videoNode, &VideoNode::textureReady, window(), &QQuickWindow::update, Qt::QueuedConnection);
    }
    emit sourceChanged(value);
}

#include "OutputUI.moc"
