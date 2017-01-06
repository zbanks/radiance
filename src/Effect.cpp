#include "Effect.h"
#include "main.h"
#include <QFile>

Effect::Effect(RenderContext *context)
    : VideoNode(context),
    m_fboIndex(0),
    m_intensity(0),
    m_previous(0),
    m_blankPreviewFbo(0) {
}

void Effect::initialize() {
    QSize size = uiSettings->previewSize();

    delete m_displayPreviewFbo;
    m_displayPreviewFbo = new QOpenGLFramebufferObject(size);

    delete m_renderPreviewFbo;
    m_renderPreviewFbo = new QOpenGLFramebufferObject(size);

    delete m_blankPreviewFbo;
    m_blankPreviewFbo = new QOpenGLFramebufferObject(size);
}

void Effect::paint() {
    QSize size = uiSettings->previewSize();

    glClearColor(0, 0, 0, 0);
    glViewport(0, 0, size.width(), size.height());
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    QOpenGLFramebufferObject *previousPreviewFbo;
    VideoNode *prev = previous();
    if(prev == 0) {
        resizeFbo(&m_blankPreviewFbo, size);
        m_blankPreviewFbo->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        previousPreviewFbo = m_blankPreviewFbo;
    } else {
        previousPreviewFbo = prev->m_previewFbo;
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

        m_previewFbo = m_previewFbos.at(m_fboIndex);
    } else {
        m_previewFbo = previousPreviewFbo;
    }
    m_programLock.unlock();

    blitToRenderFbo();
}

Effect::~Effect() {
    beforeDestruction();
    foreach(QOpenGLShaderProgram *p, m_programs) delete p;
    foreach(QOpenGLFramebufferObject *fbo, m_previewFbos) delete fbo;
    m_programs.clear();
    m_previewFbos.clear();
    m_previewFbo = 0; // This points to one of m_previewFbos
}

QSet<VideoNode*> Effect::dependencies() {
    QSet<VideoNode*> d;
    VideoNode* p = previous();
    if(p) d.insert(p);
    return d;
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

qreal Effect::intensity() {
    qreal result;
    m_intensityLock.lock();
    result = m_intensity;
    m_intensityLock.unlock();
    return result;
}

VideoNode *Effect::previous() {
    VideoNode *result;
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

void Effect::setPrevious(VideoNode *value) {
    // TODO take context lock??
    m_previousLock.lock();
    m_previous = value;
    m_previousLock.unlock();
    m_context->m_contextLock.unlock();
}
