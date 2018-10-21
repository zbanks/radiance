#include "QQuickLightOutputPreview.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

// Renderer

class LightOutputRenderer : public QQuickFramebufferObject::Renderer
{
public:
    LightOutputRenderer() {
    }

    void render() override {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        // This is a minimal OpenGL example from Qt's website
        f->glClearColor(c, 0, 0, c);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        c += 0.01f * dir;
        if (c >= 1.0f || c <= 0.0f)
            dir *= -1;
        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    float c = 0;
    int dir = 1;
};

// QQuickItem

QQuickFramebufferObject::Renderer *QQuickLightOutputPreview::createRenderer() const {
    return new LightOutputRenderer;
}

VideoNode *QQuickLightOutputPreview::videoNode() {
    return m_videoNode;
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

