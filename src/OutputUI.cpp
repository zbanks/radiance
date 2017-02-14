#include "OutputUI.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

class OutputWindow : public QOpenGLWindow, protected QOpenGLFunctions {
    Q_OBJECT

// OpenGL Events
public:
    OutputWindow(OutputUI *outputUI)
        : QOpenGLWindow(QOpenGLContext::globalShareContext())
        , m_outputUI(outputUI)
        , m_program(0) {
        showFullScreen();
    }

    ~OutputWindow() {
        makeCurrent();
        teardownGL();
    }
 
    void initializeGL() {
        // Initialize OpenGL Backend
        initializeOpenGLFunctions();

        // Set global information
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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

    void resizeGL(int width, int height) {
    }

    void paintGL() {
        glClear(GL_COLOR_BUFFER_BIT);
        QMutexLocker locker(&m_outputUI->m_sourceLock);

        if(m_outputUI->m_source != NULL) {
            auto m_videoNode = m_outputUI->m_source->m_videoNode;
            m_videoNode->swap(m_videoNode->context()->outputFboIndex());
            if(m_videoNode->m_displayFbos[m_videoNode->context()->outputFboIndex()] != NULL) {

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
                glBindTexture(GL_TEXTURE_2D, m_videoNode->m_displayFbos[m_videoNode->context()->outputFboIndex()]->texture());
                m_program->setAttributeArray(0, GL_FLOAT, values, 2);
                m_program->setUniformValue("iResolution", size());
                m_program->setUniformValue("iFrame", 0);
                m_program->enableAttributeArray(0);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                m_program->disableAttributeArray(0);
                m_program->release();
            }
        }
        glFinish();
        update();
    }

    void teardownGL() {
    }

    OutputUI *m_outputUI;
    QOpenGLShaderProgram *m_program;
};

// OutputUI

OutputUI::OutputUI() : m_source(0) {
    m_outputWindow = new OutputWindow(this);
}

void OutputUI::show() {
    m_outputWindow->show();
}

void OutputUI::hide() {
    m_outputWindow->hide();
}

VideoNodeUI *OutputUI::source() {
    QMutexLocker locker(&m_sourceLock);
    return m_source;
}

void OutputUI::setSource(VideoNodeUI *value) {
    {
        QMutexLocker locker(&m_sourceLock);
        if(m_source == value) return;
        m_source = value;
    }
    emit sourceChanged(value);
}

#include "OutputUI.moc"
