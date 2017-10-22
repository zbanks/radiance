#include "GraphicalDisplay.h"
#include "main.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

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
                                           "in vec4 vertices;\n"
                                           "out vec2 coords;\n"
                                           "void main() {\n"
                                           "    gl_Position = vertices;\n"
                                           "    coords = vertices.xy;\n"
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

            float values[] = {
                -1, -1,
                1, -1,
                -1, 1,
                1, 1
            };

            m_program->bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_1D, audio->m_waveformTexture->textureId());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_1D, audio->m_waveformBeatsTexture->textureId());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_1D, audio->m_spectrumTexture->textureId());
            m_program->setAttributeArray(0, GL_FLOAT, values, 2);
            m_program->setUniformValue("iResolution", framebufferObject()->size());
            m_program->setUniformValue("iWaveform", 0);
            m_program->setUniformValue("iBeats", 1);
            m_program->setUniformValue("iSpectrum", 2);
            m_program->enableAttributeArray(0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            m_program->disableAttributeArray(0);
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
