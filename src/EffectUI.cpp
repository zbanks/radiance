#include "EffectUI.h"
#include "Effect.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QtQuick/QQuickFramebufferObject>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOffscreenSurface>
#include <QtQuick/QSGSimpleTextureNode>
#include <QGuiApplication>

class TextureNode : public QObject, public QSGSimpleTextureNode {
    Q_OBJECT

public:
    TextureNode(QQuickWindow *window)
        : m_id(0)
        , m_size(0, 0)
        , m_texture(0)
        , m_window(window)
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
    void textureInUse();
    void pendingNewTexture();

public slots:

    // This function gets called on the FBO rendering thread and will store the
    // texture id and size and schedule an update on the window.
    void newTexture(int id, const QSize &size) {
        m_mutex.lock();
        m_id = id;
        m_size = size;
        m_mutex.unlock();

        // We cannot call QQuickWindow::update directly here, as this is only allowed
        // from the rendering thread or GUI thread.
        emit pendingNewTexture();
    }

    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode() {
        m_mutex.lock();
        int newId = m_id;
        QSize size = m_size;
        m_id = 0;
        m_mutex.unlock();
        if (newId) {
            delete m_texture;
            m_texture = m_window->createTextureFromId(newId, size, QQuickWindow::TextureHasAlphaChannel);
            setTexture(m_texture);

            markDirty(DirtyMaterial);

            // This will notify the rendering thread that the texture is now being rendered
            // and it can start rendering to the other one.
            emit textureInUse();
        }
    }

private:

    int m_id;
    QSize m_size;

    QMutex m_mutex;

    QSGTexture *m_texture;
    QQuickWindow *m_window;
};

// EffectUI

EffectUI::EffectUI() : m_intensity(0), m_renderer(0), m_previous(0), previewFbo(0) {
    setFlag(ItemHasContents, true);
    m_renderer = new Effect(this);
}

void EffectUI::ready() {
    connect(window(), &QQuickWindow::sceneGraphInvalidated, m_renderer, &Effect::shutDown, Qt::QueuedConnection);

    update();
}

QSGNode *EffectUI::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    TextureNode *node = static_cast<TextureNode *>(oldNode);

    if (!renderContext->context->shareContext()) {
        QOpenGLContext *current = window()->openglContext();
        // Some GL implementations requres that the currently bound context is
        // made non-current before we set up sharing, so we doneCurrent here
        // and makeCurrent down below while setting up our own context.
        current->doneCurrent();
        renderContext->share(current);
        current->makeCurrent(window());

        QMetaObject::invokeMethod(this, "ready");
        return 0;
    }

    if (!node) {
        node = new TextureNode(window());

        // When a new texture is ready on the rendering thread, we use a direct connection to
        // the texture node to let it know a new texture can be used. The node will then
        // emit pendingNewTexture which we bind to QQuickWindow::update to schedule a redraw.

        connect(m_renderer, &Effect::textureReady, node, &TextureNode::newTexture, Qt::DirectConnection);
        connect(node, &TextureNode::pendingNewTexture, window(), &QQuickWindow::update, Qt::QueuedConnection);
        connect(window(), &QQuickWindow::beforeRendering, node, &TextureNode::prepareNode, Qt::DirectConnection);
        connect(node, &TextureNode::textureInUse, this, &EffectUI::renderFinished, Qt::DirectConnection);

        // Get the production of FBO textures started.
        // TEMPORARY
        QMetaObject::invokeMethod(m_renderer, "render", Qt::QueuedConnection);
    }

    node->setRect(boundingRect());

    return node;
}

qreal EffectUI::intensity() {
    return m_intensity;
}

QString EffectUI::source() {
    return m_source;
}

EffectUI *EffectUI::previous() {
    return m_previous;
}

void EffectUI::setIntensity(qreal value) {
    if(value > 1) value = 1;
    if(value < 0) value = 0;
    m_intensity = value;
    emit intensityChanged(value);
}

void EffectUI::setSource(QString value) {
    m_source = value;
    emit sourceChanged(value);
}

void EffectUI::setPrevious(EffectUI *value) {
    m_previous = value;
    emit previousChanged(value);
}

void EffectUI::nextFrame() {
    // TODO: if still rendering previous frame, skip
    QMetaObject::invokeMethod(m_renderer, "render", Qt::QueuedConnection);
}

#include "EffectUI.moc"
