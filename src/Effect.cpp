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

class EffectRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    EffectRenderer(Effect *e)
        : e(e),
        surface(0),
        context(0),
        m_renderFbo(0),
        m_displayFbo(0),
        m_program(0) {
    }

    QOffscreenSurface *surface;
    QOpenGLContext *context;

public slots:
    void renderNext() {
        context->makeCurrent(surface);
        QSize size;

        if (!m_renderFbo) {
            // Initialize the buffers and renderer
            size = uiSettings->previewSize();
            QOpenGLFramebufferObjectFormat format;
            format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
            m_renderFbo = new QOpenGLFramebufferObject(size, format);
            m_displayFbo = new QOpenGLFramebufferObject(size, format);
        } else if (m_renderFbo->size() != settings->value("effect/previewSize", QSize(300, 300)).value<QSize>()) {
            size = uiSettings->previewSize();
            delete m_renderFbo;
            QOpenGLFramebufferObjectFormat format;
            format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
            m_renderFbo = new QOpenGLFramebufferObject(size, format);
        }

        if (!m_program) {
            initializeOpenGLFunctions();

            m_program = new QOpenGLShaderProgram();
            m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                               "attribute highp vec4 vertices;"
                                               "varying highp vec2 coords;"
                                               "void main() {"
                                               "    gl_Position = vertices;"
                                               "    coords = vertices.xy;"
                                               "}");
            m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                               "uniform lowp float t;"
                                               "varying highp vec2 coords;"
                                               "void main() {"
                                               "    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));"
                                               "    i = smoothstep(t - 0.8, t + 0.8, i);"
                                               "    i = floor(i * 20.) / 20.;"
                                               "    gl_FragColor = vec4(coords * .5 + .5, i, i);"
                                               "}");

            m_program->bindAttributeLocation("vertices", 0);
            m_program->link();

        }

        m_renderFbo->bind();

        // RENDER HERE
        m_program->bind();

        m_program->enableAttributeArray(0);

        float values[] = {
            -1, -1,
            1, -1,
            -1, 1,
            1, 1
        };
        m_program->setAttributeArray(0, GL_FLOAT, values, 2);
        m_program->setUniformValue("t", (float)e->intensity());

        glViewport(0, 0, size.width(), size.height());

        glDisable(GL_DEPTH_TEST);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_program->disableAttributeArray(0);
        m_program->release();

        // RENDER HERE

        // We need to flush the contents to the FBO before posting
        // the texture to the other thread, otherwise, we might
        // get unexpected results.
        context->functions()->glFlush();

        m_renderFbo->bindDefault();
        qSwap(m_renderFbo, m_displayFbo);

        emit textureReady(m_displayFbo->texture(), size);
    }

    void shutDown()
    {
        context->makeCurrent(surface);
        delete m_renderFbo;
        delete m_displayFbo;
        context->doneCurrent();
        delete context;

        // schedule this to be deleted only after we're done cleaning up
        surface->deleteLater();

        // Stop event processing, move the thread to GUI and make sure it is deleted.
        moveToThread(QGuiApplication::instance()->thread());
    }

signals:
    void textureReady(int id, const QSize &size);

private:
    QOpenGLFramebufferObject *m_renderFbo;
    QOpenGLFramebufferObject *m_displayFbo;
    QOpenGLShaderProgram *m_program;
    Effect *e;
};

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
            // note: include QQuickWindow::TextureHasAlphaChannel if the rendered content
            // has alpha.
            m_texture = m_window->createTextureFromId(newId, size);
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

// Effect

Effect::Effect() : m_intensity(0), m_renderer(0) {
    setFlag(ItemHasContents, true);
    m_renderer = new EffectRenderer(this);
}

void Effect::ready() {
    m_renderer->surface = new QOffscreenSurface();
    m_renderer->surface->setFormat(m_renderer->context->format());
    m_renderer->surface->create();

    m_renderer->moveToThread(renderThread);

    connect(window(), &QQuickWindow::sceneGraphInvalidated, m_renderer, &EffectRenderer::shutDown, Qt::QueuedConnection);

    update();
}

QSGNode *Effect::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    TextureNode *node = static_cast<TextureNode *>(oldNode);

    if (!m_renderer->context) {
        QOpenGLContext *current = window()->openglContext();
        // Some GL implementations requres that the currently bound context is
        // made non-current before we set up sharing, so we doneCurrent here
        // and makeCurrent down below while setting up our own context.
        current->doneCurrent();

        m_renderer->context = new QOpenGLContext();
        m_renderer->context->setFormat(current->format());
        m_renderer->context->setShareContext(current);
        m_renderer->context->create();
        m_renderer->context->moveToThread(renderThread);

        current->makeCurrent(window());

        QMetaObject::invokeMethod(this, "ready");
        return 0;
    }

    if (!node) {
        node = new TextureNode(window());

        // When a new texture is ready on the rendering thread, we use a direct connection to
        // the texture node to let it know a new texture can be used. The node will then
        // emit pendingNewTexture which we bind to QQuickWindow::update to schedule a redraw.

        connect(m_renderer, &EffectRenderer::textureReady, node, &TextureNode::newTexture, Qt::DirectConnection);
        connect(node, &TextureNode::pendingNewTexture, window(), &QQuickWindow::update, Qt::QueuedConnection);
        connect(window(), &QQuickWindow::beforeRendering, node, &TextureNode::prepareNode, Qt::DirectConnection);

        // TEMPORARY
        connect(node, &TextureNode::textureInUse, m_renderer, &EffectRenderer::renderNext, Qt::QueuedConnection);

        // Get the production of FBO textures started.
        // TEMPORARY
        QMetaObject::invokeMethod(m_renderer, "renderNext", Qt::QueuedConnection);
    }

    node->setRect(boundingRect());

    return node;
}

qreal Effect::intensity() {
    return m_intensity;
}

QString Effect::source() {
    return m_source;
}

void Effect::setIntensity(qreal value) {
    if(value > 1) value = 1;
    if(value < 0) value = 0;
    m_intensity = value;
    emit intensityChanged(value);
}

void Effect::setSource(QString value) {
    m_source = value;
    emit sourceChanged(value);
}

// EffectRenderer

//EffectRenderer::EffectRenderer() : m_program(0) {
//    initializeOpenGLFunctions();
//
//    m_program = new QOpenGLShaderProgram();
//    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
//                                       "attribute highp vec4 vertices;"
//                                       "varying highp vec2 coords;"
//                                       "void main() {"
//                                       "    gl_Position = vertices;"
//                                       "    coords = vertices.xy;"
//                                       "}");
//    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
//                                       "uniform lowp float t;"
//                                       "varying highp vec2 coords;"
//                                       "void main() {"
//                                       "    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));"
//                                       "    i = smoothstep(t - 0.8, t + 0.8, i);"
//                                       "    i = floor(i * 20.) / 20.;"
//                                       "    gl_FragColor = vec4(coords * .5 + .5, i, i);"
//                                       "}");
//
//    m_program->bindAttributeLocation("vertices", 0);
//    m_program->link();
//}
//
//EffectRenderer::~EffectRenderer() {
//    if(m_program) {
//        delete m_program;
//        m_program = 0;
//    }
//}
//
//void EffectRenderer::render() {
//    m_program->bind();
//
//    m_program->enableAttributeArray(0);
//
//    float values[] = {
//        -1, -1,
//        1, -1,
//        -1, 1,
//        1, 1
//    };
//    m_program->setAttributeArray(0, GL_FLOAT, values, 2);
//    m_program->setUniformValue("t", (float) 0.5);
//
//    //glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
//
//    glDisable(GL_DEPTH_TEST);
//
//    glClearColor(0, 0, 0, 1);
//    glClear(GL_COLOR_BUFFER_BIT);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//
//    m_program->disableAttributeArray(0);
//    m_program->release();
//
//    //m_window->resetOpenGLState();
//
//    update();
//}
//
//QOpenGLFramebufferObject *EffectRenderer::createFramebufferObject(const QSize &size) {
//    QOpenGLFramebufferObjectFormat format;
//    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
//    format.setSamples(4);
//    return new QOpenGLFramebufferObject(size, format);
//}

#include "Effect.moc"
