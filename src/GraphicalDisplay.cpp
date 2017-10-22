#include "GraphicalDisplay.h"
#include "main.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

class GraphicalDisplayRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
    QOpenGLShaderProgram *m_program;

public:
    GraphicalDisplayRenderer()
        : m_program(0)
        , m_fragmentShader() {
        initializeOpenGLFunctions();
    }

    ~GraphicalDisplayRenderer() {
        delete m_program;
    }

protected:
    void changeProgram(QString fragmentShader) {
        m_fragmentShader = fragmentShader;

        auto program = new QOpenGLShaderProgram();
        if(!program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 150\n"
                                           "out vec2 uv;\n"
                                           "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));\n"
                                           "void main() {\n"
                                           "    vec2 vertex = varray[gl_VertexID];\n"
                                           "    gl_Position = vec4(vertex,0.,1.);\n"
                                           "    uv = 0.5 * ( vertex + 1.);\n"
                                           "}\n")) goto err;
        if(!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader)) goto err;
        program->bindAttributeLocation("vertices", 0);
        if(!program->link()) goto err;

        delete m_program;
        m_program = program;
        return;
err:
        qDebug() << "Error setting shader program";
        delete program;
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        auto fmt = QOpenGLFramebufferObjectFormat{};
        return new QOpenGLFramebufferObject(size,fmt);
    }

    void synchronize(QQuickFramebufferObject *item) override {
        auto waveformUI = static_cast<GraphicalDisplay *>(item);
        auto fs = waveformUI->fragmentShader();
        if(fs != m_fragmentShader) changeProgram(fs);
    }

    void render() override {
        if(m_program != 0) {
            audio->renderGraphics();

            glClearColor(0, 0, 0, 0);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            m_program->bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_1D, audio->m_waveformTexture->textureId());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_1D, audio->m_waveformBeatsTexture->textureId());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_1D, audio->m_spectrumTexture->textureId());
            m_program->setUniformValue("iResolution", framebufferObject()->size());
            m_program->setUniformValue("iWaveform", 0);
            m_program->setUniformValue("iBeats", 1);
            m_program->setUniformValue("iSpectrum", 2);
            QOpenGLVertexArrayObject vao;
            vao.create();
            vao.bind();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            m_program->release();
            glActiveTexture(GL_TEXTURE0);
        }
        update();
    }

    QString m_fragmentShader;
};

QString GraphicalDisplay::fragmentShader() {
    QMutexLocker locker(&m_fragmentShaderLock);
    return m_fragmentShader;
}

void GraphicalDisplay::setFragmentShader(QString fragmentShader) {
    {
        QMutexLocker locker(&m_fragmentShaderLock);
        m_fragmentShader = fragmentShader;
    }
    emit fragmentShaderChanged(fragmentShader);
}

QQuickFramebufferObject::Renderer *GraphicalDisplay::createRenderer() const {
    return new GraphicalDisplayRenderer();
}
