#include "Effect.h"
#include "main.h"
#include <QFile>

Effect::Effect(RenderContext *context, int n_outputs)
    : VideoNode(context, n_outputs),
    m_fboIndex(0),
    m_intensity(0),
    m_previous(0),
    m_intermediateFbos(n_outputs),
    m_blankFbos(n_outputs) {
}

void Effect::initialize() {
    for(int i=0; i<m_fbos.size(); i++) {
        QSize size = uiSettings->previewSize(); // TODO

        delete m_displayFbos.at(i);
        m_displayFbos[i] = new QOpenGLFramebufferObject(size);

        delete m_renderFbos.at(i);
        m_renderFbos[i] = new QOpenGLFramebufferObject(size);

        delete m_blankFbos.at(i);
        m_blankFbos[i] = new QOpenGLFramebufferObject(size);
    }
}

void Effect::paint() {
    {
        QMutexLocker locker(&m_programLock);
        for(int i=0; i<m_fbos.size(); i++) {
            QSize size = uiSettings->previewSize(); // TODO

            glClearColor(0, 0, 0, 0);
            glViewport(0, 0, size.width(), size.height());
            glDisable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_BLEND);

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

            if(m_programs.count() > 0) {
                float values[] = {
                    -1, -1,
                    1, -1,
                    -1, 1,
                    1, 1
                };

                if(m_regenerateFbos) {
                    foreach(QOpenGLFramebufferObject *f, m_intermediateFbos.at(i)) delete f;
                    m_intermediateFbos[i].clear();
                    for(int j = 0; j < m_programs.count() + 1; j++) {
                        m_intermediateFbos[i].append(new QOpenGLFramebufferObject(size));
                        m_fboIndex = 0;
                    }
                }

                for(int j = m_programs.count() - 1; j >= 0; j--) {
                    resizeFbo(&m_intermediateFbos[i][(m_fboIndex + 1) % (m_programs.count() + 1)], size);
                    auto target = m_intermediateFbos.at(i).at((m_fboIndex + 1) % (m_programs.count() + 1));
                    auto p = m_programs.at(j);

                    p->bind();
                    target->bind();

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, previousFbo->texture());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, m_intermediateFbos.at(i).at(m_fboIndex)->texture());

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

                m_fboIndex = (m_fboIndex + 1) % m_intermediateFbos.at(i).count();
                QOpenGLFramebufferObject::bindDefault();

                m_fbos[i] = m_intermediateFbos.at(i).at(m_fboIndex);
            } else {
                m_fbos[i] = previousFbo;
            }
        }
        m_regenerateFbos = false;
    }
    blitToRenderFbo();
}

Effect::~Effect() {
    beforeDestruction();
    foreach(QOpenGLShaderProgram *p, m_programs) delete p;
    for(int i=0; i<m_fbos.size(); i++) {
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
    QString filename = QString("../resources/effects/%1.glsl").arg(name);
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Could not open \"%1\"").arg(filename);
        return false;
    }

    QTextStream s1(&file);
    QString s = s1.readAll();

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
