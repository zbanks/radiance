#include "EffectNode.h"
#include "RenderContext.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "main.h"

#include <iostream>

EffectNode::EffectNode()
    : VideoNode(renderContext, 1)
    , m_openGLWorker(this)
    , m_intensity(0)
    , m_initialized(false)
    , m_textureIndex(m_inputCount) {
    connect(&m_openGLWorker, &EffectNodeOpenGLWorker::initialized, this, &EffectNode::onInitialized);
    Q_ASSERT(QMetaObject::invokeMethod(&m_openGLWorker, "initialize"));
}

EffectNode::~EffectNode() {
}

void EffectNode::initialize(QOpenGLFunctions *glFuncs) {
    //qDebug() << "Initializing EffectNode in thread" << QThread::currentThread();
    bool result = loadProgram(m_name);
    if(!result) {
        emit deleteMe();
        return;
    }

    m_intermediate.clear();
    for(int i = 0; i<m_context->chainCount(); i++) {
        m_intermediate.append(QVector<QSharedPointer<QOpenGLTexture>>());
        for(int j = 0; j < m_programs.count() + 1; j++) {
            auto tex = QSharedPointer<QOpenGLTexture>(new QOpenGLTexture(QOpenGLTexture::Target2D));
            auto size = m_context->chainSize(i);
            tex->setSize(size.width(), size.height());
            //tex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::NoPixelType);
            tex->bind();
	        glFuncs->glTexImage2D(tex->target(), 0, GL_RGBA, tex->width(), tex->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            m_intermediate[i].append(tex);
        }
    }

    m_fbos.clear();
    for(int i=0; i<m_context->chainCount(); i++) {
        m_fbos.append(QSharedPointer<FramebufferObject>(new FramebufferObject(glFuncs)));
    }
}

void EffectNode::onInitialized() {
    m_initialized = true;
}

void EffectNode::paint(int chain, QVector<QSharedPointer<QOpenGLTexture>> inputTextures) {
    //qDebug() << "calling paint" << chain << inputTextures.count();
    if(!m_initialized) {
        m_textures[chain] = nullptr; // Uninitialized
        //qDebug() << "but uninitialized :(";
        return;
    }

    glClearColor(0, 0, 1., 1.);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    {
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

        QOpenGLFramebufferObject fbo(300, 300);

        {
            auto size = m_context->chainSize(chain);

            glViewport(0, 0, size.width(), size.height());
            auto previousTex = inputTextures.at(0);

                for(int j = m_programs.count() - 1; j >= 0; j--) {
                    //qDebug() << "Rendering shader" << j << "onto" << (m_textureIndex.at(chain) + j + 1) % (m_programs.count() + 1);
                    auto target = m_intermediate.at(chain).at((m_textureIndex.at(chain) + j + 1) % (m_programs.size() + 1));
                    auto & p = m_programs.at(j);

                    p->bind();
                    m_fbos[chain]->bind();
                    m_fbos[chain]->setTexture(target->textureId());

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, previousTex->textureId());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, m_context->noiseTexture(chain)->textureId());
                    for(int k=0; k<m_programs.size(); k++) {
                        glActiveTexture(GL_TEXTURE2 + k);
                        glBindTexture(GL_TEXTURE_2D, m_intermediate.at(chain).at((m_textureIndex.at(chain) + k + (j < k)) % (m_programs.size() + 1))->textureId());
                        //qDebug() << "Bind" << (m_textureIndex.at(chain) + k + (j < k)) % (m_programs.count() + 1) << "as chan" << k;
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

                    //fbo.toImage().save("out.png");
                    //qDebug() << "saved out.png";

                    m_fbos[chain]->release();
                    p->release();
                }

                m_textures[chain] = m_intermediate.at(chain).at((m_textureIndex.at(chain) + 1) % (m_programs.size() + 1));
                //qDebug() << "Output is" << ((m_textureIndex.at(chain) + 1) % (m_programs.count() + 1));
        }
        m_textureIndex[chain] = (m_textureIndex.at(chain) + 1) % (m_programs.size() + 1);
    }
}

// Call this to load shader code into this Effect.
// This function is thread-safe, avoid calling this in the render thread.
// A current OpenGL context is required.
// Returns true if the program was loaded successfully
bool EffectNode::loadProgram(QString name) {
    QFile header_file("../resources/glsl/effect_header.glsl");
    if(!header_file.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Could not open \"../resources/effect_header.glsl\"");
        return false;
    }
    QTextStream headerStream(&header_file);
    auto headerString = headerStream.readAll();

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
        m_programs.append(QSharedPointer<QOpenGLShaderProgram>::create());
        auto && program = m_programs.back();
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
    if(m_programs.empty()) {
        qDebug() << QString("No shaders found for \"%1\"").arg(name);
        return false;
    }

    return true;
err:
    return false;
}

qreal EffectNode::intensity() {
    //Q_ASSERT(QThread::currentThread() == thread()); // XXX put this back
    return m_intensity;
}

void EffectNode::setIntensity(qreal value) {
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

QString EffectNode::name() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_name;
}

void EffectNode::setName(QString name) {
    Q_ASSERT(QThread::currentThread() == thread());
    m_name = name;
    emit nameChanged(name);
}

// EffectNodeOpenGLWorker methods

EffectNodeOpenGLWorker::EffectNodeOpenGLWorker(EffectNode *p)
    : OpenGLWorker(openGLWorkerContext)
    , m_p(p) {
}

void EffectNodeOpenGLWorker::initialize() {
    makeCurrent();
    m_p->initialize(glFuncs());
    emit initialized();
}

