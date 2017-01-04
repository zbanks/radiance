#include "Effect.h"
#include "EffectUI.h"
#include "main.h"

Effect::Effect(RenderContext *context)
    : m_context(context),
    m_displayPreviewFbo(0),
    m_renderPreviewFbo(0),
    m_blankPreviewFbo(0),
    m_fboIndex(0),
    m_intensity(0),
    m_previous(0),
    m_prevContext(0),
    m_previewUpdated(false) {
    moveToThread(context->thread());
}

void Effect::initialize() {
    QSize size = uiSettings->previewSize();

    initializeOpenGLFunctions(); // Placement of this function is black magic to me

    if (m_displayPreviewFbo != NULL) delete m_displayPreviewFbo;
    m_displayPreviewFbo = new QOpenGLFramebufferObject(size);

    if (m_renderPreviewFbo != NULL) delete m_renderPreviewFbo;
    m_renderPreviewFbo = new QOpenGLFramebufferObject(size);

    if (m_blankPreviewFbo != NULL) delete m_blankPreviewFbo;
    m_blankPreviewFbo = new QOpenGLFramebufferObject(size);

    m_prevSource = "";
    m_previewUpdated = false;
}

void Effect::render() {
    m_context->makeCurrent();
    if(m_prevContext != m_context->context) {
        initialize();
        m_prevContext = m_context->context;
    }

    QSize size = uiSettings->previewSize();

    glClearColor(0, 0, 0, 0);
    glViewport(0, 0, size.width(), size.height());
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    QOpenGLFramebufferObject *previousPreviewFbo;
    Effect * prev = previous();
    if(prev == 0) {
        resizeFbo(&m_blankPreviewFbo, size);
        m_blankPreviewFbo->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        previousPreviewFbo = m_blankPreviewFbo;
    } else {
        prev->render();
        m_context->makeCurrent();
        previousPreviewFbo = prev->previewFbo;
    }

    m_programLock.lock();
    if(m_programs.count() > 0) {
        float values[] = {
            -1, -1,
            1, -1,
            -1, 1,
            1, 1
        };

        if(m_regeneratePreviewFbos) {
            foreach(QOpenGLFramebufferObject *f, m_previewFbos) delete f;
            m_previewFbos.clear();
            for(int i = 0; i < m_programs.count() + 1; i++) {
                m_previewFbos.append(new QOpenGLFramebufferObject(size));
                m_fboIndex = 0;
            }
            m_regeneratePreviewFbos = false;
        }

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
            float intense = intensity();
            p->setUniformValue("iIntensity", intense);
            p->setUniformValue("iFrame", 0);
            p->setUniformValue("iChannelP", 1);
            p->enableAttributeArray(0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            p->disableAttributeArray(0);
            p->release();
        }

        m_fboIndex = (m_fboIndex + 1) % m_previewFbos.count();
        QOpenGLFramebufferObject::bindDefault();

        previewFbo = m_previewFbos.at(m_fboIndex);
    } else {
        previewFbo = previousPreviewFbo;
    }

    // We need to flush the contents to the FBO before posting
    // the texture to the other thread, otherwise, we might
    // get unexpected results.
    m_context->flush();
    m_programLock.unlock();

    m_previewLock.lock();
    resizeFbo(&m_renderPreviewFbo, size);
    QOpenGLFramebufferObject::blitFramebuffer(m_renderPreviewFbo, previewFbo);
    // Flush again, hopefully calling this before taking
    // the lock makes this one go very fast
    m_context->flush();
    m_previewUpdated = true;
    m_previewLock.unlock();

    emit textureReady();
}

// This function is called from the rendering thread
// to get the latest preview frame in m_displayPreviewFbo.
// It returns true if there is a new frame
bool Effect::swapPreview() {
    m_previewLock.lock();
    bool previewUpdated = m_previewUpdated;
    if(previewUpdated) {
        resizeFbo(&m_displayPreviewFbo, m_renderPreviewFbo->size());
        QOpenGLFramebufferObject::blitFramebuffer(m_displayPreviewFbo, m_renderPreviewFbo);
        m_previewUpdated = false;
    }
    m_previewLock.unlock();
    return previewUpdated;
}

Effect::~Effect() {
    m_context->makeCurrent();
    delete m_renderPreviewFbo;
    delete m_displayPreviewFbo;
    foreach(QOpenGLShaderProgram *p, m_programs) delete p;
    foreach(QOpenGLFramebufferObject *fbo, m_previewFbos) delete fbo;
    m_renderPreviewFbo = 0;
    m_displayPreviewFbo = 0;
    m_programs.clear();
    m_previewFbos.clear();
    // Stop event processing, move the thread to GUI and make sure it is deleted.
    //moveToThread(QGuiApplication::instance()->thread());
}

// Call this to load shader code into this Effect.
// This function is thread-safe, avoid calling this in the render thread.
// A current OpenGL context is required.
// Returns true if the program was loaded successfully
bool Effect::loadProgram(QString name) {
    QString filename = QString("../resources/effects/%1.glsl").arg(name);
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Could not open \"%1\"").arg(filename);
        return false;
    }

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

    m_programLock.lock();
    foreach(QOpenGLShaderProgram *p, m_programs) delete p;
    m_programs.clear();
    m_programs.append(program);
    m_regeneratePreviewFbos = true;
    m_programLock.unlock();

    return true;
}

void Effect::resizeFbo(QOpenGLFramebufferObject **fbo, QSize size) {
    if((*fbo)->size() != size) {
        delete *fbo;
        *fbo = new QOpenGLFramebufferObject(size);
    }
}

qreal Effect::intensity() {
    qreal result;
    m_intensityLock.lock();
    result = m_intensity;
    m_intensityLock.unlock();
    return result;
}

Effect *Effect::previous() {
    Effect *result;
    m_previousLock.lock();
    result = m_previous;
    m_previousLock.unlock();
    return result;
}

void Effect::setIntensity(qreal value) {
    m_intensityLock.lock();
    if(value > 1) value = 1;
    if(value < 0) value = 0;
    m_intensity = value;
    m_intensityLock.unlock();
    emit intensityChanged(value);
}

void Effect::setPrevious(Effect *value) {
    m_previousLock.lock();
    m_previous = value;
    m_previousLock.unlock();
    emit previousChanged(value);
}

bool Effect::isMaster() {
    m_masterLock.lock();
    return m_context->master() == this;
    m_masterLock.unlock();
}

void Effect::setMaster(bool set) {
    m_masterLock.lock();
    Effect *master = m_context->master();
    if(!set && master == this) {
        m_context->setMaster(NULL);
    } else if(set && master != this) {
        m_context->setMaster(this);
    }
    m_masterLock.unlock();
}
