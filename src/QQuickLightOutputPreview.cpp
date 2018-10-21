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
            m_shader->bind();
            m_vao.bind();

            auto outputSize = videoNode->chain().size();

            auto factorFitX = (float)outputSize.height() * m_viewportSize.width() / outputSize.width() / m_viewportSize.height();
            auto factorFitY = (float)outputSize.width() * m_viewportSize.height() / outputSize.height() / m_viewportSize.width();

            factorFitX = qMax(factorFitX, 1.f);
            factorFitY = qMax(factorFitY, 1.f);
            auto centerX = 0.5 * (1. - factorFitX);
            auto centerY = 0.5 * (1. - factorFitY);

            QMatrix4x4 projection;
            projection.ortho(centerX, centerX + factorFitX, centerY + factorFitY, centerY, -1., 1.);
            m_shader->setUniformValue("mvp", projection);

            auto posAttr = m_shader->attributeLocation("posAttr");
            auto colAttr = m_shader->attributeLocation("colAttr");
            glEnableVertexAttribArray(posAttr);
            glEnableVertexAttribArray(colAttr);

            {
                QMutexLocker locker(videoNode->bufferLock());
                auto pixelCount = videoNode->pixelCount();
                auto colors = videoNode->colorsBuffer();
                auto lookupCoordinates = videoNode->lookupCoordinatesBuffer();

                lookupCoordinates.bind();
                glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 0, 0);
                colors.bind();
                glVertexAttribPointer(colAttr, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
                colors.release();
                glDrawArrays(GL_POINTS, 0, pixelCount);
            }

            glDisableVertexAttribArray(posAttr);
            glDisableVertexAttribArray(colAttr);
            m_vao.release();
            m_shader->release();
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
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    QSharedPointer<QOpenGLShaderProgram> loadShader() {
        auto vertexString = QString{
            "#version 150\n"
            "attribute vec2 posAttr;\n"
            "attribute vec4 colAttr;\n"
            "out vec4 col;\n"
            "uniform mat4 mvp;\n"
            "void main() {\n"
            "   col = colAttr;\n"
            "   gl_Position = mvp * vec4(posAttr, 0., 1.);\n"
            "   gl_PointSize = 20;\n"
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

        m_vao.create();

        return shader;
    }

    void initialize() {
        m_shader = loadShader();
    }

    QSharedPointer<QOpenGLShaderProgram> m_shader;
    bool m_initialized{};
    QQuickLightOutputPreview *m_p{};
    QOpenGLVertexArrayObject m_vao;
    QSize m_viewportSize;
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

