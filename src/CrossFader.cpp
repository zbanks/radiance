#include "CrossFader.h"
#include "RenderContext.h"
#include <QFile>

CrossFader::CrossFader(RenderContext *context)
    : VideoNodeOld(context)
    , m_parameter(0)
    , m_left()
    , m_right()
    , m_blankFbos(context->outputCount())
    , m_program() {
}

void CrossFader::initialize() {
    for(int i=0; i<m_context->outputCount(); i++) {
        auto size = m_context->fboSize(i);
        resizeFbo(displayFbo(i),size);
        resizeFbo(outputFbo(i),size);
        resizeFbo(renderFbo(i),size);
        resizeFbo(blankFbo(i),size);
    }
}

std::shared_ptr<QOpenGLFramebufferObject> & CrossFader::blankFbo(int i)
{
    return m_blankFbos[i];
}
const std::shared_ptr<QOpenGLFramebufferObject> & CrossFader::blankFbo(int i) const
{
    return m_blankFbos.at(i);
}
void CrossFader::paint() {
    auto program = decltype(m_program){};
    {
        QMutexLocker lock(&m_programLock);
        program = m_program;
    }
    if(program) {
        program->bind();
        float param = parameter();
        program->setUniformValue("iParameter", param);
//        float param = parameter();
//        m_program->setUniformValue("iParameter", param);
        auto node_left = left();
        auto node_right = right();
        for(auto i=0; i<m_context->outputCount(); i++) {
            auto size = m_context->fboSize(i);

            glClearColor(0, 0, 0, 0);
            glViewport(0, 0, size.width(), size.height());
            glDisable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_BLEND);

            auto leftPreviewFbo = std::shared_ptr<QOpenGLFramebufferObject>{};
            auto rightPreviewFbo = std::shared_ptr<QOpenGLFramebufferObject>{};

            if(!node_left) {
                resizeFbo(blankFbo(i), size);
                blankFbo(i)->bind();
                glClear(GL_COLOR_BUFFER_BIT);
                leftPreviewFbo = blankFbo(i);
            } else {
                leftPreviewFbo = node_left->outputFbo(i);
            }

            if(!node_right) {
                resizeFbo(blankFbo(i), size);
                blankFbo(i)->bind();
                glClear(GL_COLOR_BUFFER_BIT);
                rightPreviewFbo = blankFbo(i);
            } else {
                rightPreviewFbo = node_right->outputFbo(i);
            }
            resizeFbo(outputFbo(i), size);
            outputFbo(i)->bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, leftPreviewFbo->texture());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, rightPreviewFbo->texture());

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            QOpenGLFramebufferObject::bindDefault();
        }
        m_program->release();
    } else {
        for(int i=0; i<m_context->outputCount(); i++) {
            auto size = m_context->fboSize(i);

            glClearColor(0, 0, 0, 0);
            glViewport(0, 0, size.width(), size.height());
            glDisable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_BLEND);
        }
    }
    blitToRenderFbo();
}

CrossFader::~CrossFader() {
    beforeDestruction();
}

// Call this function once at the beginning from the render thread.
// A current OpenGL context is required.
// Returns true if the crossfader loaded successfully
bool CrossFader::load() {
    auto filename = QString{"../resources/glsl/crossfader.glsl"};
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Could not open \"%1\"").arg(filename);
        return false;
    }

    QTextStream s1(&file);
    auto s = s1.readAll();

    auto program = std::make_shared<QOpenGLShaderProgram>();
    program->addShaderFromSourceCode(QOpenGLShader::Vertex,
        RenderContext::defaultVertexShaderSource());
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, s);
    program->link();
    program->bind();
    program->setUniformValue("iLeft", 0);
    program->setUniformValue("iRight", 1);

    {
        QMutexLocker lock(&m_programLock);
        m_program.swap(program);
    }
    return true;
}

qreal CrossFader::parameter() {
    QMutexLocker lock(&m_parameterLock);
    return m_parameter;
}

VideoNodeOld *CrossFader::left() {
    QMutexLocker lock(&m_leftLock);
    return m_left;
}

VideoNodeOld *CrossFader::right() {
    QMutexLocker lock(&m_rightLock);
    return m_right;
}

void CrossFader::setParameter(qreal value) {
    if(value > 1) value = 1;
    if(value < 0) value = 0;
    {
        QMutexLocker lock(&m_parameterLock);
        if(m_parameter = value)
            return;
        m_parameter = value;
    }
    emit parameterChanged(value);
}

void CrossFader::setLeft(VideoNodeOld *value) {
    {
        QMutexLocker lock(&m_context->m_contextLock);
        {
            QMutexLocker lock(&m_leftLock);
            if(m_left == value)
                return;
            m_left = value;
        }
        emit leftChanged(value);
    }
}

void CrossFader::setRight(VideoNodeOld *value) {
    {
        QMutexLocker lock(&m_context->m_contextLock);
        {
            QMutexLocker lock(&m_rightLock);
            if(m_right == value)
                return;
            m_right = value;
        }
        emit rightChanged(value);
    }
}

QSet<VideoNodeOld*> CrossFader::dependencies() {
    auto d = QSet<VideoNodeOld*>{};
    for(auto p : { left(),right()})
        if(p) d.insert(p);
    return d;
}
