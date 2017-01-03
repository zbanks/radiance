#include "Effect.h"
#include "EffectUI.h"
#include "main.h"

#include <QThread>

Effect::Effect()
    : m_displayPreviewFbo(0),
    m_renderPreviewFbo(0),
    m_blankPreviewFbo(0),
    m_fboIndex(0),
    m_intensity(0),
    m_previous(0) {
    moveToThread(renderContext->thread());
}

void Effect::render() {
    renderContext->makeCurrent();
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

    QString s = source();
    if (m_prevSource != s) {
        m_prevSource = s;
        loadProgram(s);
    }

    QOpenGLFramebufferObject *previousPreviewFbo;
    Effect * prev = previous();
    if(prev == 0) {
        resizeFbo(&m_blankPreviewFbo, size);
        m_blankPreviewFbo->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        previousPreviewFbo = m_blankPreviewFbo;
    } else {
        prev->render();
        previousPreviewFbo = prev->previewFbo;
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
            p->setUniformValue("iIntensity", (float)intensity());
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
        previewFbo = m_previewFbos.at(m_fboIndex);
    } else {
        QOpenGLFramebufferObject::blitFramebuffer(m_renderPreviewFbo, previousPreviewFbo);
        previewFbo = previousPreviewFbo;
    }
 
    // We need to flush the contents to the FBO before posting
    // the texture to the other thread, otherwise, we might
    // get unexpected results.
    renderContext->flush();

    qSwap(m_renderPreviewFbo, m_displayPreviewFbo);

    emit textureReady(m_displayPreviewFbo->texture(), size);
}

void Effect::shutDown() {
    renderContext->makeCurrent();
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

void Effect::loadProgram(QString filename) {
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

QString Effect::source() {
    QString result;
    m_sourceLock.lock();
    result = m_source;
    m_sourceLock.unlock();
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

void Effect::setSource(QString value) {
    m_sourceLock.lock();
    m_source = value;
    m_sourceLock.unlock();
    emit sourceChanged(value);
}

void Effect::setPrevious(Effect *value) {
    m_previousLock.lock();
    m_previous = value;
    m_previousLock.unlock();
    emit previousChanged(value);
}
