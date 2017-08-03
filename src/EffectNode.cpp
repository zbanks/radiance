#include "EffectNode.h"
#include "RenderContext.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "main.h"

Effect::Effect(Model *model)
    : VideoNode(model),
    m_intensity(0) {
}

void Effect::paint(QOpenGLFramebufferObject *outputFbo) {
    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    {
        QMutexLocker locker(&m_programLock);
        auto chanTex = std::make_unique<GLuint[]>(m_programs.size());
        std::iota(&chanTex[0], &chanTex[0] + m_programs.size(), 2);
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
            auto previousFbo = std::shared_ptr<QOpenGLFramebufferObject>{};
//            QOpenGLFramebufferObject *previousFbo;
            auto prev = previous();
            if(prev == 0) {
                previousFbo = m_context->blankFbo();
            } else {
                previousFbo = prev->outputFbo(i);
            }

            if(m_regenerateFbos) {
//                foreach(auto f, m_intermediateFbos.at(i))
//                    delete f;
                m_intermediateFbos[i].clear();
                m_intermediateFbos[i].resize(m_programs.size() + 1);
                for(auto & f : m_intermediateFbos[i]) {
                    resizeFbo(f,size);
                }
               m_fboIndex = 0;
            }

            for(int j=0; j < m_programs.size() + 1; j++)
                resizeFbo(m_intermediateFbos[i][j], size);
            if(m_programs.size() > 0) {
                for(int j = m_programs.size() - 1; j >= 0; j--) {
                    //qDebug() << "Rendering shader" << j << "onto" << (m_fboIndex + j + 1) % (m_programs.count() + 1);
                    auto target = m_intermediateFbos.at(i).at((m_fboIndex + j + 1) % (m_programs.size() + 1));
                    auto & p = m_programs.at(j);

                    p->bind();
                    target->bind();

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, previousFbo->texture());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, m_context->noiseTexture(i)->textureId());
                    for(int k=0; k<m_programs.size(); k++) {
                        glActiveTexture(GL_TEXTURE2 + k);
                        glBindTexture(GL_TEXTURE_2D, m_intermediateFbos.at(i).at((m_fboIndex + k + (j < k)) % (m_programs.size() + 1))->texture());
                        //qDebug() << "Bind" << (m_fboIndex + k + (j < k)) % (m_programs.count() + 1) << "as chan" << k;
                    }

                    auto intense = qreal(intensity());
                    m_intensityIntegral = fmod(m_intensityIntegral + intense / FPS, MAX_INTEGRAL);
                    p->setUniformValue("iIntensity", GLfloat(intense));
                    p->setUniformValue("iIntensityIntegral", GLfloat(m_intensityIntegral));
                    p->setUniformValue("iStep", GLfloat(step));
                    p->setUniformValue("iTime", GLfloat(time));
                    p->setUniformValue("iFPS",  GLfloat(FPS));
                    p->setUniformValue("iAudio", QVector4D(GLfloat(audioLow),GLfloat(audioMid),GLfloat(audioHi),GLfloat(audioLevel)));
                    p->setUniformValue("iFrame", 0);
                    p->setUniformValue("iNoise", 1);
                    p->setUniformValue("iResolution", GLfloat(size.width()), GLfloat(size.height()));
                    p->setUniformValueArray("iChannel", &chanTex[0], m_programs.size());
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    p->release();
                }

                m_fbos[i] = m_intermediateFbos.at(i).at((m_fboIndex + 1) % (m_programs.size() + 1));
                //qDebug() << "Output is" << ((m_fboIndex + 1) % (m_programs.count() + 1));
            } else {
                m_fbos[i] = previousFbo;
            }
        }
        m_fboIndex = (m_fboIndex + 1) % (m_programs.size() + 1);
        m_regenerateFbos = false;
    }
    blitToRenderFbo();
}

Effect::~Effect() {
    beforeDestruction();
    m_programs.clear();
//    foreach(QOpenGLShaderProgram *p, m_programs)
//        delete p;
    m_fbos.clear();
    m_intermediateFbos.clear();
}

QSet<VideoNodeOld*> Effect::dependencies() {
    QSet<VideoNodeOld*> d;
    if(auto p = previous())
        d.insert(p);
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
    auto headerString = headerStream.readAll();

    auto programs = std::vector<std::unique_ptr<QOpenGLShaderProgram> >{};

    for(int i=0;;i++) {
        auto filename = QString("../resources/effects/%1.%2.glsl").arg(name).arg(i);
        QFile file(filename);

        QFileInfo check_file(filename);
        if(!(check_file.exists() && check_file.isFile()))
            break;

        if(!file.open(QIODevice::ReadOnly)) {
            qDebug() << QString("Could not open \"%1\"").arg(filename);
            goto err;
        }
        QTextStream stream(&file);
        auto s = headerString + stream.readAll();
        programs.emplace_back(std::make_unique<QOpenGLShaderProgram>());
        auto && program = programs.back();
        if(!program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                       "#version 130\n"
                                       "#extension GL_ARB_shading_language_420pack : enable\n"
                                       "const vec2 varray[4] = { vec2( 1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.)};\n"
                                       "varying vec2 coords;\n"
                                       "void main() {"
                                       "    vec2 vertex = varray[gl_VertexID];\n"
                                       "    gl_Position = vec4(vertex,0.,1.);\n"
                                       "    coords = vertex;\n"
                                       "}")) goto err;
        if(!program->addShaderFromSourceCode(QOpenGLShader::Fragment, s))
            goto err;
        if(!program->link())
            goto err;
    }
    if(programs.empty()) {
        qDebug() << QString("No shaders found for \"%1\"").arg(name);
        return false;
    }

    {
        // Take the program lock so we don't render while swapping out programs
        QMutexLocker locker(&m_programLock);
        m_programs.swap(programs);
        m_regenerateFbos = true;
    }

    return true;
err:
    return false;
}

qreal Effect::intensity() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_intensity;
}

VideoNodeOld *Effect::previous() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_previous;
}

void Effect::setIntensity(qreal value) {
    Q_ASSERT(QThread::currentThread() == thread());
    {
        if(value > 1) value = 1;
        if(value < 0) value = 0;
        if(m_intensity == value)
            return;
        m_intensity = value;
    }
    emit intensityChanged(value);
}

void Effect::setPrevious(VideoNodeOld *value) {
    Q_ASSERT(QThread::currentThread() == thread());
    m_previous = value;
}

QStringList EffectList::effectNames() {
    auto filters = QStringList{} << QString{"*.0.glsl"};
    QDir dir("../resources/effects/");
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);
    return dir.entryList().replaceInStrings(".0.glsl","");
}
