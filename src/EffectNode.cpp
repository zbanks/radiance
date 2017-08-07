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

// TODO initialization sometimes runs on top of render()
// need to fix this!!!
void EffectNode::initialize(QOpenGLFunctions *glFuncs) {
    bool result = loadProgram(m_name);
    if(!result) {
        emit deleteMe();
        return;
    }

    m_intermediate.clear();
    for(int i = 0; i<m_context->chainCount(); i++) {
        m_intermediate.append(QVector<QSharedPointer<QOpenGLFramebufferObject>>());
    }
}

void EffectNode::onInitialized() {
    m_initialized = true;
    qDebug() << "its initialized!";
}

void EffectNode::paint(int chain, QVector<GLuint> inputTextures) {
    QMutexLocker locker(&m_renderLocks[chain]);
    //qDebug() << "calling paint" << this << chain << inputTextures.count();
    if(!m_initialized) {
        m_textures[chain] = 0; // Uninitialized
        //qDebug() << "but uninitialized :(";
        return;
    }

    // FBO creation must happen here, and not in initialize,
    // because FBOs are not shared among contexts.
    // Textures are, however, so in the future maybe we can move
    // texture creation to initialize()
    // and leave lightweight FBO creation here
    if(m_intermediate.at(chain).isEmpty()) {
        for(int i = 0; i < m_programs.count() + 1; i++) {
            auto fbo = QSharedPointer<QOpenGLFramebufferObject>(new QOpenGLFramebufferObject(size(chain)));
            m_intermediate[chain].append(fbo);
        }
    }

    glClearColor(0, 0, 0, 0);
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

        auto size = m_context->chainSize(chain);
        glViewport(0, 0, size.width(), size.height());

        auto previousTex = inputTextures.at(0);

        for(int j = m_programs.count() - 1; j >= 0; j--) {
            //qDebug() << "Rendering shader" << j << "onto" << (m_textureIndex.at(chain) + j + 1) % (m_programs.count() + 1);
            auto fboIndex = (m_textureIndex.at(chain) + j + 1) % (m_programs.size() + 1);
            auto & p = m_programs.at(j);

            p->bind();
            m_intermediate.at(chain).at(fboIndex)->bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, previousTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_context->noiseTexture(chain));
            for(int k=0; k<m_programs.size(); k++) {
                glActiveTexture(GL_TEXTURE2 + k);
                glBindTexture(GL_TEXTURE_2D, m_intermediate.at(chain).at((m_textureIndex.at(chain) + k + (j < k)) % (m_programs.size() + 1))->texture());
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
            m_intermediate.at(chain).at(fboIndex)->release();
            //m_intermediate.at(chain).at(fboIndex)->toImage().save(QString("out_%1.png").arg(m_intermediate.at(chain).at(fboIndex)->texture()));
            p->release();
            glActiveTexture(GL_TEXTURE0); // Very important to reset OpenGL state for scene graph rendering
        }

        m_textures[chain] = m_intermediate.at(chain).at((m_textureIndex.at(chain) + 1) % (m_programs.size() + 1))->texture();
        //qDebug() << "Output texture ID is" << m_textures[chain];
        //qDebug() << "Output is" << ((m_textureIndex.at(chain) + 1) % (m_programs.count() + 1));
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
    QMutexLocker locker(&m_intensityLock);
    return m_intensity;
}

void EffectNode::setIntensity(qreal value) {
    {
        QMutexLocker locker(&m_intensityLock);
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
    m_initialized = false; // TODO weird concurrency issues here
    Q_ASSERT(QMetaObject::invokeMethod(&m_openGLWorker, "initialize"));
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

