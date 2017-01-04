#include "EffectUI.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>

class TextureNode : public QObject, public QSGSimpleTextureNode {
    Q_OBJECT

public:
    TextureNode(QQuickWindow *window, Effect *effect)
        : m_id(0)
        , m_size(0, 0)
        , m_texture(0)
        , m_window(window)
        , m_effect(effect)
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
        if(m_effect->swapPreview()) {
            int newId = m_effect->m_displayPreviewFbo->texture();
            QSize size = m_effect->m_displayPreviewFbo->size();
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
    Effect *m_effect;
};

// EffectUI

EffectUI::EffectUI() : m_renderer(0), m_previous(0) {
    setFlag(ItemHasContents, true);
    m_renderer = new Effect(renderContext);
    connect(m_renderer, &Effect::intensityChanged, this, &EffectUI::intensityChanged);
    connect(m_renderer, &Effect::sourceChanged, this, &EffectUI::sourceChanged);
}

qreal EffectUI::intensity() {
    return m_renderer->intensity();
}

QString EffectUI::source() {
    return m_renderer->source();
}

EffectUI *EffectUI::previous() {
    return m_previous;
}

void EffectUI::setIntensity(qreal value) {
    m_renderer->setIntensity(value);
}

void EffectUI::setSource(QString value) {
    m_renderer->setSource(value);
}

void EffectUI::setPrevious(EffectUI *value) {
    m_previous = value;
    if(m_previous == NULL) {
        m_renderer->setPrevious(NULL);
    } else {
        m_renderer->setPrevious(value->m_renderer);
    }
    emit previousChanged(value);
}

bool EffectUI::isMaster() {
    return m_renderer->isMaster();
}

void EffectUI::setMaster(bool set) {
    m_renderer->setMaster(set);
}

QSGNode *EffectUI::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
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
        node = new TextureNode(window(), m_renderer);

        // When a new texture is ready on the rendering thread, we use a direct connection to
        // the texture node to let it know a new texture can be used. The node will then
        // emit pendingNewTexture which we bind to QQuickWindow::update to schedule a redraw.

        connect(m_renderer, &Effect::textureReady, window(), &QQuickWindow::update, Qt::QueuedConnection);
        connect(window(), &QQuickWindow::beforeRendering, node, &TextureNode::prepareNode, Qt::DirectConnection);
        connect(window(), &QQuickWindow::frameSwapped, m_renderer, &Effect::nextFrame, Qt::QueuedConnection);
    }

    node->setRect(boundingRect());

    return node;
}

#include "EffectUI.moc"
