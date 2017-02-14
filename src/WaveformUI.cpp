#include "WaveformUI.h"
#include "main.h"

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>

class WaveformRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
    QOpenGLShaderProgram *m_program;

public:
    WaveformRenderer()
        : m_program(0)
        , m_fragmentShader() {
        initializeOpenGLFunctions();
    }

    ~WaveformRenderer() {
        delete m_program;
    }

protected:
    void changeProgram(QString fragmentShader) {
        m_fragmentShader = fragmentShader;

        auto program = new QOpenGLShaderProgram();
        if(!program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute highp vec4 vertices;"
                                           "varying highp vec2 coords;"
                                           "void main() {"
                                           "    gl_Position = vertices;"
                                           "    coords = vertices.xy;"
                                           "}")) goto err;
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
        return new QOpenGLFramebufferObject(size);
    }

    void synchronize(QQuickFramebufferObject *item) override {
        auto waveformUI = static_cast<WaveformUI *>(item);
        auto fs = waveformUI->fragmentShader();
        if(fs != m_fragmentShader) changeProgram(fs);
    }

    void render() override {
        if(m_program != 0) {
            audio->renderWaveform();

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
            m_program->setAttributeArray(0, GL_FLOAT, values, 2);
            m_program->setUniformValue("iResolution", framebufferObject()->size());
            m_program->setUniformValue("iWaveform", 0);
            m_program->enableAttributeArray(0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            m_program->disableAttributeArray(0);
            m_program->release();
        }
        update();
    }

    QString m_fragmentShader;
};

QString WaveformUI::fragmentShader() {
    QMutexLocker locker(&m_fragmentShaderLock);
    return m_fragmentShader;
}

void WaveformUI::setFragmentShader(QString fragmentShader) {
    {
        QMutexLocker locker(&m_fragmentShaderLock);
        m_fragmentShader = fragmentShader;
    }
    emit fragmentShaderChanged(fragmentShader);
}

QQuickFramebufferObject::Renderer *WaveformUI::createRenderer() const {
    return new WaveformRenderer();
}