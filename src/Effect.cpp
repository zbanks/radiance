#include "Effect.h"
#include "EffectUI.h"
#include "main.h"

Effect::Effect(EffectUI *e)
    : e(e),
    m_displayPreviewFbo(0),
    m_renderPreviewFbo(0),
    m_blankPreviewFbo(0),
    m_fboIndex(0) {
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

    if (m_source != e->source()) {
        m_source = e->source();
        loadProgram(m_source);
    }

    QOpenGLFramebufferObject *previousPreviewFbo;
    // TODO locking???
    EffectUI * previous = e->previous();
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
