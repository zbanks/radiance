#include "VideoNodeUI.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QQuickFramebufferObject>

class VideoNodeRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    VideoNodeRenderer(VideoNode *videoNode)
        : m_videoNode(videoNode)
        , m_program(0) {

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
                                        "uniform vec2 iResolution;"
                                        "uniform sampler2D iFrame;"
                                        "void main(void) {"
                                        "    vec2 uv = gl_FragCoord.xy / iResolution;"
                                        "    gl_FragColor = texture2D(iFrame, uv);"
                                        "}");
        program->bindAttributeLocation("vertices", 0);
        program->link();

        m_program = program;
    }

protected:
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        return new QOpenGLFramebufferObject(size);
    }

    void render() override {
        m_videoNode->swap(m_videoNode->context()->previewFboIndex());
        if(m_videoNode->m_displayFbos[m_videoNode->context()->previewFboIndex()] != NULL) {

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
            glBindTexture(GL_TEXTURE_2D, m_videoNode->m_displayFbos[m_videoNode->context()->previewFboIndex()]->texture());
            m_program->setAttributeArray(0, GL_FLOAT, values, 2);
            m_program->setUniformValue("iResolution", framebufferObject()->size());
            m_program->setUniformValue("iFrame", 0);
            m_program->enableAttributeArray(0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            m_program->disableAttributeArray(0);
            m_program->release();
        }
        update();
    }

    VideoNode *m_videoNode;
    QOpenGLShaderProgram *m_program;
};

// VideoNodeUI

VideoNodeUI::VideoNodeUI()
    : m_videoNode(0) {
    //setMirrorVertically(true);
}

VideoNodeUI::~VideoNodeUI() {
    delete m_videoNode;
    m_videoNode = 0;
}

QQuickFramebufferObject::Renderer *VideoNodeUI::createRenderer() const {
    return new VideoNodeRenderer(m_videoNode);
}
