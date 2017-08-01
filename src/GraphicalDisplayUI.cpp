#include "GraphicalDisplayUI.h"
#include "main.h"

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>

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
            RenderContextOld::defaultVertexShaderSource()))
            goto err;
        if(!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader))
            goto err;
        if(!program->link())
            goto err;

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
        auto waveformUI = static_cast<GraphicalDisplayUI *>(item);
        auto fs = waveformUI->fragmentShader();
        if(fs != m_fragmentShader) changeProgram(fs);
    }

    void render() override {
        if(m_program) {
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
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            m_program->release();
        }
        update();
    }

    QString m_fragmentShader;
};

QString GraphicalDisplayUI::fragmentShader() {
    QMutexLocker locker(&m_fragmentShaderLock);
    return m_fragmentShader;
}

void GraphicalDisplayUI::setFragmentShader(QString fragmentShader) {
    {
        QMutexLocker locker(&m_fragmentShaderLock);
        m_fragmentShader = fragmentShader;
    }
    emit fragmentShaderChanged(fragmentShader);
}

QQuickFramebufferObject::Renderer *GraphicalDisplayUI::createRenderer() const {
    return new GraphicalDisplayRenderer();
}
