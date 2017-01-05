#include "CrossFader.h"
#include "main.h"
#include <QFile>

CrossFader::CrossFader(RenderContext *context)
    : VideoNode(context),
    m_parameter(0),
    m_left(0),
    m_right(0),
    m_blankPreviewFbo(0),
    m_previewFbo(0),
    m_program(0) {
}

void CrossFader::initialize() {
    QSize size = uiSettings->previewSize();

    delete m_displayPreviewFbo;
    m_displayPreviewFbo = new QOpenGLFramebufferObject(size);

    delete m_renderPreviewFbo;
    m_renderPreviewFbo = new QOpenGLFramebufferObject(size);

    delete m_blankPreviewFbo;
    m_blankPreviewFbo = new QOpenGLFramebufferObject(size);

    delete m_previewFbo;
    m_previewFbo = new QOpenGLFramebufferObject(size);
}

void CrossFader::paint() {
    QSize size = uiSettings->previewSize();

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
        resizeFbo(&m_blankPreviewFbo, size);
        m_blankPreviewFbo->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        leftPreviewFbo = m_blankPreviewFbo;
    } else {
        node_left->render();
        leftPreviewFbo = node_left->m_renderPreviewFbo;
    }

    if(node_right == 0) {
        resizeFbo(&m_blankPreviewFbo, size);
        m_blankPreviewFbo->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        rightPreviewFbo = m_blankPreviewFbo;
    } else {
        node_right->render();
        rightPreviewFbo = node_right->m_renderPreviewFbo;
    }

    float values[] = {
        -1, -1,
        1, -1,
        -1, 1,
        1, 1
    };

    resizeFbo(&m_previewFbo, size);

    m_previewFbo->bind();
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

    blitToPreviewFbo(m_previewFbo);
}

CrossFader::~CrossFader() {
    m_context->makeCurrent();
    delete m_program;
    delete m_previewFbo;
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
    m_leftLock.lock();
    m_left = value;
    m_leftLock.unlock();
    emit leftChanged(value);
}

void CrossFader::setRight(VideoNode *value) {
    m_rightLock.lock();
    m_right = value;
    m_rightLock.unlock();
    emit rightChanged(value);
}

