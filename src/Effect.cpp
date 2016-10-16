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
        m_displayPreviewFbo(0),
        m_renderPreviewFbo(0),
        m_blankPreviewFbo(0),
        m_fboIndex(0) {
        moveToThread(renderThread);
    }

public slots:
    void render() {
        renderThread->makeCurrent();
        QSize size = uiSettings->previewSize();

        if (!m_renderPreviewFbo) {
            initializeOpenGLFunctions(); // Placement of this function is black magic to me
            m_displayPreviewFbo = new QOpenGLFramebufferObject(size);
            m_renderPreviewFbo = new QOpenGLFramebufferObject(size);
            m_blankPreviewFbo = new QOpenGLFramebufferObject(size);
        }

        glClearColor(0, 0, 0, 0);
        glViewport(0, 0, size.width(), size.height());
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_BLEND);

        if (m_source != e->source()) {
            m_source = e->source();
            loadProgram(m_source);
        }

        QOpenGLFramebufferObject *previousPreviewFbo;
        // TODO locking???
        Effect * previous = e->previous();
        if(previous == 0) {
            resizeFbo(&m_blankPreviewFbo, size);
            m_blankPreviewFbo->bind();
            glClear(GL_COLOR_BUFFER_BIT);
            previousPreviewFbo = m_blankPreviewFbo;
        } else {
            previous->m_renderer->render();
            previousPreviewFbo = previous->previewFbo;
        }

        if(m_programs.count() > 0) {
            float values[] = {
                -1, -1,
                1, -1,
                -1, 1,
                1, 1
            };

            for(int i = m_programs.count() - 1; i >= 0; i--) {
                resizeFbo(&m_previewFbos[(m_fboIndex + 1) % m_previewFbos.count()], size);
                QOpenGLFramebufferObject *target = m_previewFbos.at((m_fboIndex + 1) % m_previewFbos.count());
                QOpenGLShaderProgram * p = m_programs.at(i);

                p->bind();
                target->bind();

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, previousPreviewFbo->texture());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, m_previewFbos.at(m_fboIndex)->texture());

                p->setAttributeArray(0, GL_FLOAT, values, 2);
                p->setUniformValue("iIntensity", (float)e->intensity());
                p->setUniformValue("iFrame", 0);
                p->setUniformValue("iChannelP", 1);
                p->enableAttributeArray(0);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                p->disableAttributeArray(0);
                p->release();
            }

            m_fboIndex = (m_fboIndex + 1) % m_previewFbos.count();
            QOpenGLFramebufferObject::bindDefault();

            resizeFbo(&m_renderPreviewFbo, size);
            QOpenGLFramebufferObject::blitFramebuffer(m_renderPreviewFbo, m_previewFbos.at(m_fboIndex));
            e->previewFbo = m_previewFbos.at(m_fboIndex);
        } else {
            QOpenGLFramebufferObject::blitFramebuffer(m_renderPreviewFbo, previousPreviewFbo);
            e->previewFbo = previousPreviewFbo;
        }
 
        // We need to flush the contents to the FBO before posting
        // the texture to the other thread, otherwise, we might
        // get unexpected results.
        renderThread->flush();

        qSwap(m_renderPreviewFbo, m_displayPreviewFbo);

        emit textureReady(m_displayPreviewFbo->texture(), size);
    }

    void shutDown()
    {
        renderThread->makeCurrent();
        delete m_renderPreviewFbo;
        delete m_displayPreviewFbo;
        foreach(QOpenGLShaderProgram *p, m_programs) delete p;
        foreach(QOpenGLFramebufferObject *fbo, m_previewFbos) delete fbo;
        m_renderPreviewFbo = 0;
        m_displayPreviewFbo = 0;
        m_programs.clear();
        m_previewFbos.clear();
        // Stop event processing, move the thread to GUI and make sure it is deleted.
        moveToThread(QGuiApplication::instance()->thread());
    }

signals:
    void textureReady(int id, const QSize &size);

private:
    QVector<QOpenGLFramebufferObject *> m_previewFbos;
    QVector<QOpenGLShaderProgram *> m_programs;
    QOpenGLFramebufferObject * m_displayPreviewFbo;
    QOpenGLFramebufferObject * m_renderPreviewFbo;
    QOpenGLFramebufferObject * m_blankPreviewFbo;
    Effect *e;
    QString m_source;
    int m_fboIndex;

    void loadProgram(QString filename) {
        QFile file(filename);
        file.open(QIODevice::ReadOnly);

        QTextStream s1(&file);
        QString s = s1.readAll();

        QOpenGLShaderProgram *program = new QOpenGLShaderProgram();
        program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute highp vec4 vertices;"
                                           "varying highp vec2 coords;"
                                           "void main() {"
                                           "    gl_Position = vertices;"
                                           "    coords = vertices.xy;"
                                           "}");
        program->addShaderFromSourceCode(QOpenGLShader::Fragment, s);
        program->bindAttributeLocation("vertices", 0);
        program->link();

        foreach(QOpenGLShaderProgram *p, m_programs) delete p;
        m_programs.clear();
        m_programs.append(program);

        QSize size = uiSettings->previewSize();
        for(int i = 0; i < m_programs.count() + 1; i++) {
            m_previewFbos.append(new QOpenGLFramebufferObject(size));
            m_fboIndex = 0;
        }
        m_renderPreviewFbo = new QOpenGLFramebufferObject(size);
    }

    void resizeFbo(QOpenGLFramebufferObject **fbo, QSize size) {
        if((*fbo)->size() != size) {
            delete *fbo;
            *fbo = new QOpenGLFramebufferObject(size);
        }
    }
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

// Effect

Effect::Effect() : m_intensity(0), m_renderer(0), m_previous(0), previewFbo(0) {
    setFlag(ItemHasContents, true);
    m_renderer = new EffectRenderer(this);
}

void Effect::ready() {
    connect(window(), &QQuickWindow::sceneGraphInvalidated, m_renderer, &EffectRenderer::shutDown, Qt::QueuedConnection);

    update();
}

QSGNode *Effect::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    TextureNode *node = static_cast<TextureNode *>(oldNode);

    if (!renderThread->context) {
        QOpenGLContext *current = window()->openglContext();
        // Some GL implementations requres that the currently bound context is
        // made non-current before we set up sharing, so we doneCurrent here
        // and makeCurrent down below while setting up our own context.
        current->doneCurrent();
        renderThread->makeContext(current);
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
        connect(node, &TextureNode::textureInUse, this, &Effect::renderFinished, Qt::DirectConnection);

        // Get the production of FBO textures started.
        // TEMPORARY
        QMetaObject::invokeMethod(m_renderer, "render", Qt::QueuedConnection);
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

Effect *Effect::previous() {
    return m_previous;
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

void Effect::setPrevious(Effect *value) {
    m_previous = value;
    emit previousChanged(value);
}

void Effect::nextFrame() {
    // TODO: if still rendering previous frame, skip
    QMetaObject::invokeMethod(m_renderer, "render", Qt::QueuedConnection);
}

#include "Effect.moc"
