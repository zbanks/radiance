#include "Effect.h"
#include "RenderContext.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "main.h"

Effect::Effect(RenderContext *context)
    : VideoNode(context),
    m_fboIndex(0),
    m_intensity(0),
    m_previous(0),
    m_intermediateFbos(context->outputCount()),
    m_blankFbos(context->outputCount()),
    m_regenerateFbos(true) {
}

void Effect::initialize() {
    for(int i=0; i<m_context->outputCount(); i++) {
        QSize size = m_context->fboSize(i);

        resizeFbo(&m_displayFbos[i], size);
        resizeFbo(&m_renderFbos[i], size);
        resizeFbo(&m_blankFbos[i], size);
    }
}

void Effect::paint() {
    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    {
        QMutexLocker locker(&m_programLock);
        GLuint *chanTex = new GLuint[m_programs.count()];
        std::iota(chanTex,chanTex + m_programs.count(), 1);
        auto   time = timebase->beat();
        m_realTimeLast = m_realTime;
        m_realTime     = timebase->wallTime();
        auto   step = m_realTime - m_realTimeLast;
        double audioHi = 0;
        double audioMid = 0;
        double audioLow = 0;
        double audioLevel = 0;
        audio->levels(&audioHi, &audioMid, &audioLow, &audioLevel);

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
                foreach(auto f, m_intermediateFbos.at(i)) delete f;
                m_intermediateFbos[i].clear();
                for(int j = 0; j < m_programs.count() + 1; j++) {
                    QOpenGLFramebufferObject *fbo = 0;
                    resizeFbo(&fbo, size);
                    m_intermediateFbos[i].append(fbo);
                }
                m_fboIndex = 0;
            }

            for(int j=0; j < m_programs.count() + 1; j++) resizeFbo(&m_intermediateFbos[i][j], size);

            if(m_programs.count() > 0) {
                float values[] = {
                    -1, -1,
                    1, -1,
                    -1, 1,
                    1, 1
                };

                for(int j = m_programs.count() - 1; j >= 0; j--) {
                    //qDebug() << "Rendering shader" << j << "onto" << (m_fboIndex + j + 1) % (m_programs.count() + 1);
                    auto target = m_intermediateFbos.at(i).at((m_fboIndex + j + 1) % (m_programs.count() + 1));
                    auto p = m_programs.at(j);

                    p->bind();
                    target->bind();

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, previousFbo->texture());
                    for(int k=0; k<m_programs.count(); k++) {
                        glActiveTexture(GL_TEXTURE1 + k);
                        glBindTexture(GL_TEXTURE_2D, m_intermediateFbos.at(i).at((m_fboIndex + k + (j < k)) % (m_programs.count() + 1))->texture());
                        //qDebug() << "Bind" << (m_fboIndex + k + (j < k)) % (m_programs.count() + 1) << "as chan" << k;
                    }

                    p->setAttributeArray(0, GL_FLOAT, values, 2);
                    qreal intense = intensity();
                    m_intensityIntegral = fmod(m_intensityIntegral + intense / FPS, MAX_INTEGRAL);
                    p->setUniformValue("iIntensity", GLfloat(intense));
                    p->setUniformValue("iIntensityIntegral", GLfloat(m_intensityIntegral));
                    p->setUniformValue("iStep", GLfloat(step));
                    p->setUniformValue("iTime", GLfloat(time));
                    p->setUniformValue("iFPS",  GLfloat(FPS));
                    p->setUniformValue("iAudio", QVector4D(GLfloat(audioLow),GLfloat(audioMid),GLfloat(audioHi),GLfloat(audioLevel)));
/*                    p->setUniformValue("iAudioHi", (GLfloat)audioHi);
                    p->setUniformValue("iAudioMid", (GLfloat)audioMid);
                    p->setUniformValue("iAudioLow", (GLfloat)audioLow);
                    p->setUniformValue("iAudioLevel", (GLfloat)audioLevel);*/
                    p->setUniformValue("iFrame", 0);
                    p->setUniformValue("iResolution", GLfloat( size.width()), GLfloat( size.height()));
                    p->setUniformValueArray("iChannel", chanTex, m_programs.count());
                    p->enableAttributeArray(0);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    p->disableAttributeArray(0);
                    p->release();
                }
                QOpenGLFramebufferObject::bindDefault();

                m_fbos[i] = m_intermediateFbos.at(i).at((m_fboIndex + 1) % (m_programs.count() + 1));
                //qDebug() << "Output is" << ((m_fboIndex + 1) % (m_programs.count() + 1));
            } else {
                m_fbos[i] = previousFbo;
            }
        }
        m_fboIndex = (m_fboIndex + 1) % (m_programs.count() + 1);
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
    QTextStream headerStream(&header_file);
    QString headerString = headerStream.readAll();

    QVector<QOpenGLShaderProgram *> programs;

    for(int i=0;;i++) {
        QString filename = QString("../resources/effects/%1.%2.glsl").arg(name).arg(i);
        QFile file(filename);

        QFileInfo check_file(filename);
        if(!(check_file.exists() && check_file.isFile())) break;

        if(!file.open(QIODevice::ReadOnly)) {
            qDebug() << QString("Could not open \"%1\"").arg(filename);
            goto err;
        }

        QTextStream stream(&file);
        QString s = headerString + stream.readAll();

        auto program = new QOpenGLShaderProgram();
        programs.append(program);
        if(!program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute highp vec4 vertices;"
                                           "varying highp vec2 coords;"
                                           "void main() {"
                                           "    gl_Position = vertices;"
                                           "    coords = vertices.xy;"
                                           "}")) goto err;
        if(!program->addShaderFromSourceCode(QOpenGLShader::Fragment, s)) goto err;
        program->bindAttributeLocation("vertices", 0);
        if(!program->link()) goto err;
    }
    if(programs.count() == 0) {
        qDebug() << QString("No shaders found for \"%1\"").arg(name);
        return false;
    }

    {
        QMutexLocker locker(&m_programLock);
        foreach(auto p, m_programs) delete p;
        m_programs.clear();
        foreach(auto p, programs) m_programs.append(p);
        m_regenerateFbos = true;
    }

    return true;
err:
    foreach(auto *p, programs) delete p;
    programs.clear();
    return false;
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

QStringList EffectList::effectNames() {
    QStringList filters;
    filters << "*.0.glsl";
    QDir dir("../resources/effects/");
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);

    QStringList entries = dir.entryList();
    return entries.replaceInStrings(".0.glsl", "");
}
