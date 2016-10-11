#include "effect.h"

#include <QtQuick/QQuickFramebufferObject>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>

//#include <QtQuick/QQuickWindow>
//#include <qsgsimpletexturenode.h>

// Effect

Effect::Effect() : m_intensity(0) {
}

qreal Effect::intensity() {
    return m_intensity;
}

QString Effect::source() {
    return m_source;
}

void Effect::setIntensity(qreal value) {
    if(value > 1) value = 1;
    if(value < 0) value = 0;
    m_intensity = value;
    emit intensityChanged(value);
}

void Effect::setSource(QString value) {
    m_source = value;
    emit sourceChanged(value);
}

QQuickFramebufferObject::Renderer *Effect::createRenderer() const {
    return new EffectRenderer();
}

// EffectRenderer

EffectRenderer::EffectRenderer() : m_program(0) {
    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram();
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                       "attribute highp vec4 vertices;"
                                       "varying highp vec2 coords;"
                                       "void main() {"
                                       "    gl_Position = vertices;"
                                       "    coords = vertices.xy;"
                                       "}");
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                       "uniform lowp float t;"
                                       "varying highp vec2 coords;"
                                       "void main() {"
                                       "    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));"
                                       "    i = smoothstep(t - 0.8, t + 0.8, i);"
                                       "    i = floor(i * 20.) / 20.;"
                                       "    gl_FragColor = vec4(coords * .5 + .5, i, i);"
                                       "}");

    m_program->bindAttributeLocation("vertices", 0);
    m_program->link();
}

EffectRenderer::~EffectRenderer() {
    if(m_program) {
        delete m_program;
        m_program = 0;
    }
}

void EffectRenderer::render() {
    m_program->bind();

    m_program->enableAttributeArray(0);

    float values[] = {
        -1, -1,
        1, -1,
        -1, 1,
        1, 1
    };
    m_program->setAttributeArray(0, GL_FLOAT, values, 2);
    m_program->setUniformValue("t", (float) 0.5);

    //glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    glDisable(GL_DEPTH_TEST);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->disableAttributeArray(0);
    m_program->release();

    //m_window->resetOpenGLState();

    update();
}

QOpenGLFramebufferObject *EffectRenderer::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

