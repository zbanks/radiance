#include "WaveformUI.h"
#include "main.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

class WaveformRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
    QOpenGLShaderProgram *m_program;

public:
    WaveformRenderer()
        : m_program(0) {
        initializeOpenGLFunctions();

        auto program = new QOpenGLShaderProgram();
        program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "attribute highp vec4 vertices;"
                                           "varying highp vec2 coords;"
                                           "void main() {"
                                           "    gl_Position = vertices;"
                                           "    coords = vertices.xy;"
                                           "}");
        program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                        //"// Alpha-compsite two colors, putting one on top of the other"
                                        "vec4 composite(vec4 under, vec4 over) {"
                                        "    float a_out = 1. - (1. - over.a) * (1. - under.a);"
                                        "    return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));"
                                        "}"

                                        "uniform sampler1D iWaveform;"
                                        "uniform vec2 iResolution;"

                                        "void main(void) {"
                                        "    vec2 uv = gl_FragCoord.xy / iResolution;"
                                        "    float g = uv.y * 0.5 + 0.1;"
                                        "    float w = 4.;"

                                        "    gl_FragColor = vec4(0.);"

                                        "    float freq = (uv.x - 0.5) + 0.5;"
                                        "    float mag = abs(uv.y - 0.5);"
                                        "    vec4 wav = texture1D(iWaveform, freq);"
                                        //"    //vec4 beats = texture1D(iBeats, freq);"
                                        "    vec3 wf = (wav.rgb - mag) * 90.;"
                                        "    wf = smoothstep(0., 1., wf);"
                                        "    float level = (wav.a - mag) * 90.;"
                                        "    level = (smoothstep(-5., 0., level) - smoothstep(0., 5., level));"
                                        //"    //float beat = beats.x;"
                                        //"    //beat = beat * (1. - step(1., df));"
                                        //"    //float rg = 0.5 * clamp(0., 1., d.r / 30.);"
                                        //"    //gl_FragColor = composite(gl_FragColor, vec4(0.5, 0.1, 0.1, beat));"
                                        "    gl_FragColor = composite(gl_FragColor, vec4(0.0, 0.0, 0.6, wf.b));"
                                        "    gl_FragColor = composite(gl_FragColor, vec4(0.3, 0.3, 1.0, wf.g));"
                                        "    gl_FragColor = composite(gl_FragColor, vec4(0.7, 0.7, 1.0, wf.r));"
                                        "    gl_FragColor = composite(gl_FragColor, vec4(0.7, 0.7, 0.7, level * 0.5));"
                                        //"    //gl_FragColor = composite(gl_FragColor, vec4(0.3, 0.3, 0.3, smoothstep(0., 1., df) - smoothstep(2., 5., df)));"
                                        "}");
        program->bindAttributeLocation("vertices", 0);
        program->link();

        m_program = program;
    }

    ~WaveformRenderer() {
        delete m_program;
    }

protected:
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        return new QOpenGLFramebufferObject(size);
    }

    void render() override {
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
        update();
    }
};

QQuickFramebufferObject::Renderer *WaveformUI::createRenderer() const {
    return new WaveformRenderer;
}
