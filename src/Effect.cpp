#include "Effect.h"
#include "RenderContext.h"
#include <QFile>
#include "main.h"

Effect::Effect(RenderContext *context)
    : VideoNode(context),
    m_fboIndex(0),
    m_intensity(0),
    m_previous(0),
    m_intermediateFbos(context->outputCount()),
    m_blankFbos(context->outputCount()) {
}

void Effect::initialize() {
    for(int i=0; i<m_context->outputCount(); i++) {
        QSize size = m_context->fboSize(i);

        delete m_displayFbos.at(i);
        m_displayFbos[i] = new QOpenGLFramebufferObject(size);

        delete m_renderFbos.at(i);
        m_renderFbos[i] = new QOpenGLFramebufferObject(size);

        delete m_blankFbos.at(i);
        m_blankFbos[i] = new QOpenGLFramebufferObject(size);
    }
}

void Effect::paint() {
    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    {
        QMutexLocker locker(&m_programLock);
        double time = audio->time();
        for(int i=0; i<m_context->outputCount(); i++) {
            QSize size = m_context->fboSize(i);

            glViewport(0, 0, size.width(), size.height());

            QOpenGLFramebufferObject *previousFbo;
            VideoNode *prev = previous();
            if(prev == 0) {
                resizeFbo(&m_blankFbos[i], size);
                m_blankFbos.at(i)->bind();
                glClear(GL_COLOR_BUFFER_BIT);
                previousFbo = m_blankFbos.at(i);
            } else {
                previousFbo = prev->m_fbos[i];
            }

            if(m_regenerateFbos) {
                foreach(QOpenGLFramebufferObject *f, m_intermediateFbos.at(i)) delete f;
                m_intermediateFbos[i].clear();
                for(int j = 0; j < m_programs.count() + 1; j++) {
                    m_intermediateFbos[i].append(new QOpenGLFramebufferObject(size));
                }
                m_fboIndex = 0;
            }

            if(m_programs.count() > 0) {
                float values[] = {
                    -1, -1,
                    1, -1,
                    -1, 1,
                    1, 1
                };

                int fboIndex = m_fboIndex;
                for(int j = m_programs.count() - 1; j >= 0; j--) {
                    resizeFbo(&m_intermediateFbos[i][(fboIndex + 1) % (m_programs.count() + 1)], size);
                    auto target = m_intermediateFbos.at(i).at((fboIndex + 1) % (m_programs.count() + 1));
                    auto p = m_programs.at(j);

                    p->bind();
                    target->bind();

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, previousFbo->texture());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, m_intermediateFbos.at(i).at(fboIndex)->texture());

                    p->setAttributeArray(0, GL_FLOAT, values, 2);
                    float intense = intensity();
                    p->setUniformValue("iIntensity", intense);
                    p->setUniformValue("iTime", (GLfloat)time);
                    p->setUniformValue("iFrame", 0);
                    p->setUniformValue("iResolution", (GLfloat) size.width(), (GLfloat) size.height());
                    p->setUniformValue("iChannelP", 1);
                    p->enableAttributeArray(0);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    p->disableAttributeArray(0);
                    p->release();

                    fboIndex = (fboIndex + 1) % (m_programs.count() + 1);
                }
                QOpenGLFramebufferObject::bindDefault();

                m_fbos[i] = m_intermediateFbos.at(i).at(fboIndex);
            } else {
                m_fbos[i] = previousFbo;
            }
        }
        m_fboIndex = (m_fboIndex + m_programs.count()) % (m_programs.count() + 1);
        m_regenerateFbos = false;
    }
    blitToRenderFbo();
}

Effect::~Effect() {
    beforeDestruction();
    foreach(QOpenGLShaderProgram *p, m_programs) delete p;
    for(int i=0; i<m_context->outputCount(); i++) {
        foreach(QOpenGLFramebufferObject *fbo, m_intermediateFbos.at(i)) delete fbo;
        m_intermediateFbos[i].clear();
        m_fbos[i] = 0; // This points to one of m_intermediateFbos
    }
    m_programs.clear();
}

QSet<VideoNode*> Effect::dependencies() {
    QSet<VideoNode*> d;
    if(auto p = previous()) d.insert(p);
    return d;
}

// Call this to load shader code into this Effect.
// This function is thread-safe, avoid calling this in the render thread.
// A current OpenGL context is required.
// Returns true if the program was loaded successfully
bool Effect::loadProgram(QString name) {
    QFile header_file("../resources/glsl/effect_header.glsl");
    if(!header_file.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Could not open \"../resources/effect_header.glsl\"");
        return false;
    }
    QTextStream s1(&header_file);

    QString filename = QString("../resources/effects/%1.0.glsl").arg(name);
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Could not open \"%1\"").arg(filename);
        return false;
    }

    QTextStream s2(&file);
    QString s = s1.readAll() + s2.readAll();

    auto program = new QOpenGLShaderProgram();
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

    {
        QMutexLocker locker(&m_programLock);
        foreach(QOpenGLShaderProgram *p, m_programs) delete p;
        m_programs.clear();
        m_programs.append(program);
        m_regenerateFbos = true;
    }

    return true;
}

qreal Effect::intensity() {
    QMutexLocker locker(&m_intensityLock);
    return m_intensity;
}

VideoNode *Effect::previous() {
    QMutexLocker locker(&m_previousLock);
    return m_previous;
}

void Effect::setIntensity(qreal value) {
    {
        if(value > 1) value = 1;
        if(value < 0) value = 0;
        QMutexLocker locker(&m_intensityLock);
        if(m_intensity == value)
            return;
        m_intensity = value;
    }
    emit intensityChanged(value);
}

void Effect::setPrevious(VideoNode *value) {
    QMutexLocker clocker(&m_context->m_contextLock);
    QMutexLocker plocker(&m_previousLock);
    m_previous = value;
}
