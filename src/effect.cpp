#include "effect.h"

#include <QtQuick/qquickwindow.h>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>

// Effect

Effect::Effect() : m_renderer(0), m_intensity(0) {
    connect(this, &QQuickItem::windowChanged, this, &Effect::onWindowChanged);
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

void Effect::onWindowChanged(QQuickWindow *win) {
    if (win) {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &Effect::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &Effect::cleanup, Qt::DirectConnection);
        win->setClearBeforeRendering(false);
    }
}

void Effect::sync() {
    if(!m_renderer) {
        m_renderer = new EffectRenderer();
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &EffectRenderer::paint, Qt::DirectConnection);
    }
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_renderer->setWindow(window());
}

void Effect::cleanup() {
    if(m_renderer) {
        delete m_renderer;
        m_renderer = 0;
    }
}

// EffectRenderer

EffectRenderer::EffectRenderer() : m_program(0) {
}

EffectRenderer::~EffectRenderer() {
    delete m_program;
}

void EffectRenderer::paint() {
    if(!m_program) {
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

    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    glDisable(GL_DEPTH_TEST);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->disableAttributeArray(0);
    m_program->release();

    // Not strictly needed for this example, but generally useful for when
    // mixing with raw OpenGL.
    m_window->resetOpenGLState();
}
