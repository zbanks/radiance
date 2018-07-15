#include "GraphicalDisplay.h"

#include "Audio.h"
#include "Timebase.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

class GraphicalDisplayRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
    Context * const *m_context;
    QOpenGLShaderProgram *m_program;
    QOpenGLVertexArrayObject m_vao;
    QMap<const char *, QColor> m_colors;

public:
    GraphicalDisplayRenderer(Context * const *context)
        : m_context(context)
        , m_program(0)
        , m_fragmentShader() {
        initializeOpenGLFunctions();
        m_vao.create();
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
        m_colors = waveformUI->colors();
        auto fs = waveformUI->fragmentShader();
        if(fs != m_fragmentShader) changeProgram(fs);
    }

    Context *context() {
        return *m_context;
    }

    void render() override {
        if(m_program && context() != nullptr) {
            context()->audio()->renderGraphics();

            glClearColor(0, 0, 0, 0);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            m_program->bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_1D, context()->audio()->m_waveformTexture->textureId());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_1D, context()->audio()->m_waveformBeatsTexture->textureId());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_1D, context()->audio()->m_spectrumTexture->textureId());
            m_program->setUniformValue("iResolution", framebufferObject()->size());
            m_program->setUniformValue("iWaveform", 0);
            m_program->setUniformValue("iBeats", 1);
            m_program->setUniformValue("iSpectrum", 2);
            m_program->setUniformValue("iTime", (GLfloat) context()->timebase()->beat());
            for(auto e : m_colors.keys()) {
                m_program->setUniformValue(e, m_colors.value(e));
            }
            m_vao.bind();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            m_program->release();
            m_vao.release();
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
    return new GraphicalDisplayRenderer(&m_context);
}

Context *GraphicalDisplay::context() {
    return m_context;
}

void GraphicalDisplay::setContext(Context *context) {
    if (context != m_context) {
        m_context = context;
        emit contextChanged(context);
    }
}

QMap<const char *, QColor> GraphicalDisplay::colors() const {
    QMap<const char *, QColor> result;
    auto metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); i++) {
        auto p = metaObj->property(i);
        if (p.type() == QVariant::nameToType("QColor")) {
            result.insert(p.name(), p.read(this).value<QColor>());
        }
    }
    return result;
}
