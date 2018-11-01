#include "QQuickLightOutputPreview.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QtDebug>
#include <QQuickWindow>
#include "LightOutputNode.h"

// Renderer

class QQuickLightOutputPreview;

class LightOutputRenderer
    : public QQuickFramebufferObject::Renderer
    , protected QOpenGLFunctions {

public:
    LightOutputRenderer(QQuickLightOutputPreview *p) 
        : m_p(p) {
    }

    void render() override {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        //glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_MAX, GL_MAX);
        glEnable(GL_PROGRAM_POINT_SIZE);

        auto videoNode = QSharedPointer<LightOutputNode>(m_p->videoNodeSafe());
        if (!videoNode.isNull()) {
            m_vao.bind();

            auto outputSize = videoNode->chain().size();

            auto factorFitX = (float)outputSize.height() * m_viewportSize.width() / outputSize.width() / m_viewportSize.height();
            auto factorFitY = (float)outputSize.width() * m_viewportSize.height() / outputSize.height() / m_viewportSize.width();

            factorFitX = qMax(factorFitX, 1.f);
            factorFitY = qMax(factorFitY, 1.f);
            auto centerX = 0.5 * (1. - factorFitX);
            auto centerY = 0.5 * (1. - factorFitY);
            auto marginX = (5 * m_devicePixelRatio) / m_viewportSize.width();
            auto marginY = (5 * m_devicePixelRatio) / m_viewportSize.height();

            QMatrix4x4 projection;
            projection.ortho(centerX - marginX, centerX + factorFitX + marginX, centerY + factorFitY + marginY, centerY - marginY, -1., 1.);

            auto posAttr = m_lightShader->attributeLocation("posAttr");
            auto colAttr = m_lightShader->attributeLocation("colAttr");
            {
                QMutexLocker locker(videoNode->bufferLock());
                auto background = videoNode->geometry2DTexture();
                auto pixelCount = videoNode->pixelCount();
                auto colors = videoNode->colorsBuffer();
                auto displayMode = videoNode->displayMode();
                auto lookupCoordinates = videoNode->lookupCoordinatesBuffer();
                auto physicalCoordinates = videoNode->physicalCoordinatesBuffer();

                // Draw background
                m_backgroundShader->bind();
                glActiveTexture(GL_TEXTURE0);
                background->bind();

                m_backgroundShader->setUniformValue("mvp", projection);
                m_backgroundShader->setUniformValue("background", 0);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                // Draw lights
                m_lightShader->bind();
                glEnableVertexAttribArray(posAttr);
                glEnableVertexAttribArray(colAttr);
                m_lightShader->setUniformValue("mvp", projection);
                m_lightShader->setUniformValue("dpr", (GLfloat)m_devicePixelRatio);

                if (displayMode == LightOutputNode::DisplayPhysical2D) {
                    physicalCoordinates.bind();
                } else {
                    lookupCoordinates.bind();
                }
                glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 0, 0);
                colors.bind();
                glVertexAttribPointer(colAttr, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
                colors.release();
                glDrawArrays(GL_POINTS, 0, pixelCount);
                glDisableVertexAttribArray(posAttr);
                glDisableVertexAttribArray(colAttr);
            }

            m_vao.release();
            m_lightShader->release();
        }
        glDisable(GL_PROGRAM_POINT_SIZE);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glDisable(GL_BLEND);
        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        if (!m_initialized) {
            initializeOpenGLFunctions();
            initialize();
            m_initialized = true;
        }
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        m_viewportSize = size;
        m_devicePixelRatio = m_p->window()->devicePixelRatio();
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    QSharedPointer<QOpenGLShaderProgram> loadLightShader() {
        auto vertexString = QString{
            "#version 150\n"
            "attribute vec2 posAttr;\n"
            "attribute vec4 colAttr;\n"
            "out vec4 col;\n"
            "uniform mat4 mvp;\n"
            "uniform float dpr;\n"
            "void main() {\n"
            "   col = colAttr;\n"
            "   gl_Position = mvp * vec4(posAttr, 0., 1.);\n"
            "   gl_PointSize = 10 * dpr;\n"
            "}\n"};
        auto fragmentString = QString{
            "#version 150\n"
            "in vec4 col;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "   float fadeOut = max(1. - length(2. * gl_PointCoord - 1.), 0.);\n"
            "   fragColor = col * fadeOut * fadeOut;\n"
            "}\n"};

        auto shader = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

        if (!shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
            qWarning() << "Could not compile vertex shader";
            return nullptr;
        }
        if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
            qWarning() << "Could not compile fragment shader";
            return nullptr;
        }
        if (!shader->link()) {
            qWarning() << "Could not link shader program";
            return nullptr;
        }

        return shader;
    }
    QSharedPointer<QOpenGLShaderProgram> loadBackgroundShader() {
        auto vertexString = QString{
            "#version 150\n"
            "out vec2 uv;\n"
            "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., 0.),vec2(0., 1.),vec2(0., 0.));\n"
            "uniform mat4 mvp;\n"
            "void main() {\n"
            "    vec2 vertex = varray[gl_VertexID];\n"
            "    gl_Position = mvp * vec4(vertex, 0., 1.);\n"
            "    uv = vertex;\n"
            "}\n"};
    auto fragmentString = QString{
            "#version 150\n"
            "uniform sampler2D background;\n"
            "in vec2 uv;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "    fragColor = texture(background, uv);\n"
            "}\n"};

        auto shader = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

        if (!shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
            qWarning() << "Could not compile vertex shader";
            return nullptr;
        }
        if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
            qWarning() << "Could not compile fragment shader";
            return nullptr;
        }
        if (!shader->link()) {
            qWarning() << "Could not link shader program";
            return nullptr;
        }

        return shader;
    }

    void initialize() {
        m_lightShader = loadLightShader();
        m_backgroundShader = loadBackgroundShader();
        m_vao.create();
    }

    QSharedPointer<QOpenGLShaderProgram> m_lightShader;
    QSharedPointer<QOpenGLShaderProgram> m_backgroundShader;
    bool m_initialized{};
    QQuickLightOutputPreview *m_p{};
    QOpenGLVertexArrayObject m_vao;
    QSize m_viewportSize;
    qreal m_devicePixelRatio;
};

// QQuickItem

QQuickLightOutputPreview::QQuickLightOutputPreview() {
    m_renderer = new LightOutputRenderer(this);
}

QQuickFramebufferObject::Renderer *QQuickLightOutputPreview::createRenderer() const {
    return m_renderer;
}

VideoNode *QQuickLightOutputPreview::videoNode() {
    return m_videoNode;
}

LightOutputNode *QQuickLightOutputPreview::videoNodeSafe() {
    QMutexLocker locker(&m_videoNodeLock);
    if (m_videoNode == nullptr) return nullptr;
    return static_cast<LightOutputNode *>(m_videoNode->clone());
}

void QQuickLightOutputPreview::setVideoNode(VideoNode *videoNode) {
    delete m_videoNode;
    if (videoNode != nullptr) {
        m_videoNode = videoNode->clone();
        m_videoNode->setParent(this); // Ensure C++ ownership and proper deletion
    } else {
        m_videoNode = nullptr;
    }
    emit videoNodeChanged(m_videoNode);
}

