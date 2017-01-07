#pragma once

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QMutex>
#include <QSet>

class RenderContext;

class VideoNode : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    VideoNode(RenderContext *context);
   ~VideoNode() override;
    QOpenGLFramebufferObject *m_previewFbo;
    QOpenGLFramebufferObject *m_displayPreviewFbo;
    QOpenGLFramebufferObject *m_renderPreviewFbo;
    virtual QSet<VideoNode*> dependencies();

public slots:
    void render();
    bool swapPreview();

signals:
    void textureReady();

protected:
    QOpenGLFramebufferObject *previewFbo;
    virtual void initialize() = 0;
    virtual void paint() = 0;
    void blitToRenderFbo();
    RenderContext *m_context;
    static void resizeFbo(QOpenGLFramebufferObject **fbo, QSize size);
    void beforeDestruction();

private:
    QMutex m_previewLock;
    bool m_previewUpdated;
    bool m_initialized;
};
