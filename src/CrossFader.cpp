#include "CrossFader.h"
#include "main.h"
#include <QFile>

CrossFader::CrossFader(RenderContext *context)
    : VideoNode(context)
    , m_parameter(0)
    , m_left(0)
    , m_right(0)
    , m_blankFbos(context->outputCount())
    , m_program(0) {
}

void CrossFader::initialize() {
    for(int i=0; i<m_context->outputCount(); i++) {
        QSize size = uiSettings->previewSize(); // TODO

        delete m_displayFbos.at(i);
        m_displayFbos[i] = new QOpenGLFramebufferObject(size);

        delete m_renderFbos.at(i);
        m_renderFbos[i] = new QOpenGLFramebufferObject(size);

        delete m_blankFbos.at(i);
        m_blankFbos[i] = new QOpenGLFramebufferObject(size);

        delete m_fbos.at(i);
        m_fbos[i] = new QOpenGLFramebufferObject(size);
    }
}

void CrossFader::paint() {
    for(int i=0; i<m_context->outputCount(); i++) {
        QSize size = uiSettings->previewSize(); // TODO

        glClearColor(0, 0, 0, 0);
        glViewport(0, 0, size.width(), size.height());
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_BLEND);

        QOpenGLFramebufferObject *leftPreviewFbo;
        QOpenGLFramebufferObject *rightPreviewFbo;

        VideoNode *node_left = left();
        VideoNode *node_right = right();

        if(node_left == 0) {
            resizeFbo(&m_blankFbos[i], size);
            m_blankFbos.at(i)->bind();
            glClear(GL_COLOR_BUFFER_BIT);
            leftPreviewFbo = m_blankFbos.at(i);
        } else {
            leftPreviewFbo = node_left->m_fbos.at(i);
        }

        if(node_right == 0) {
            resizeFbo(&m_blankFbos[i], size);
            m_blankFbos.at(i)->bind();
            glClear(GL_COLOR_BUFFER_BIT);
            rightPreviewFbo = m_blankFbos.at(i);
        } else {
            rightPreviewFbo = node_right->m_fbos.at(i);
        }

        float values[] = {
            -1, -1,
            1, -1,
            -1, 1,
            1, 1
        };

        resizeFbo(&m_fbos[i], size);

        m_fbos.at(i)->bind();
        m_programLock.lock();
        if(m_program) {
            m_program->bind();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, leftPreviewFbo->texture());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, rightPreviewFbo->texture());

            m_program->setAttributeArray(0, GL_FLOAT, values, 2);
            float param = parameter();
            m_program->setUniformValue("iParameter", param);
            m_program->setUniformValue("iLeft", 0);
            m_program->setUniformValue("iRight", 1);
            m_program->enableAttributeArray(0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            m_program->disableAttributeArray(0);
            m_program->release();
        } else {
            glClear(GL_COLOR_BUFFER_BIT);
        }
        m_programLock.unlock();

        QOpenGLFramebufferObject::bindDefault();
    }
    blitToRenderFbo();
}

CrossFader::~CrossFader() {
    beforeDestruction();
    delete m_program;
}

// Call this function once at the beginning from the render thread.
// A current OpenGL context is required.
// Returns true if the crossfader loaded successfully
bool CrossFader::load() {
    QString filename = "../resources/glsl/crossfader.glsl";
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
    delete m_program;
    m_program = program;
    m_programLock.unlock();

    return true;
}

qreal CrossFader::parameter() {
    qreal result;
    m_parameterLock.lock();
    result = m_parameter;
    m_parameterLock.unlock();
    return result;
}

VideoNode *CrossFader::left() {
    VideoNode *result;
    m_leftLock.lock();
    result = m_left;
    m_leftLock.unlock();
    return result;
}

VideoNode *CrossFader::right() {
    VideoNode *result;
    m_rightLock.lock();
    result = m_right;
    m_rightLock.unlock();
    return result;
}

void CrossFader::setParameter(qreal value) {
    m_parameterLock.lock();
    if(value > 1) value = 1;
    if(value < 0) value = 0;
    m_parameter = value;
    m_parameterLock.unlock();
    emit parameterChanged(value);
}

void CrossFader::setLeft(VideoNode *value) {
    m_context->m_contextLock.lock();
    m_leftLock.lock();
    m_left = value;
    m_leftLock.unlock();
    emit leftChanged(value);
    m_context->m_contextLock.unlock();
}

void CrossFader::setRight(VideoNode *value) {
    m_context->m_contextLock.lock();
    m_rightLock.lock();
    m_right = value;
    m_rightLock.unlock();
    emit rightChanged(value);
    m_context->m_contextLock.unlock();
}

QSet<VideoNode*> CrossFader::dependencies() {
    QSet<VideoNode*> d;
    VideoNode* l = left();
    VideoNode* r = right();
    if(l) d.insert(l);
    if(r) d.insert(r);
    return d;
}
