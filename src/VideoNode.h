#pragma once

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QMutex>

class RenderContext;

class VideoNode : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    VideoNode(RenderContext *context);
    virtual ~VideoNode();
    QOpenGLFramebufferObject *m_displayPreviewFbo;
    QOpenGLFramebufferObject *m_renderPreviewFbo;
 
public slots:
    void render();

    bool isMaster();
    void setMaster(bool set);

    bool swapPreview();

signals:
    void textureReady();

protected:
    QOpenGLFramebufferObject *previewFbo;
    virtual void initialize() = 0;
    virtual void paint() = 0;
    void blitToPreviewFbo(QOpenGLFramebufferObject *fbo);
    RenderContext *m_context;
    static void resizeFbo(QOpenGLFramebufferObject **fbo, QSize size);

private:
    QOpenGLContext *m_prevContext;

    QMutex m_masterLock;
    QMutex m_previewLock;

    bool m_previewUpdated;
};
