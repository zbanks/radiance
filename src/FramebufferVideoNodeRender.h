#pragma once

#include "VideoNode.h"
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTextureBlitter>
#include <QSize>

class FramebufferVideoNodeRender : public QObject {
    Q_OBJECT
    Q_PROPERTY(VideoNode *videoNode READ videoNode WRITE setVideoNode NOTIFY videoNodeChanged)
    Q_PROPERTY(int chain READ chain WRITE setChain NOTIFY chainChanged)

public:
    FramebufferVideoNodeRender();
    virtual ~FramebufferVideoNodeRender();

    VideoNode *videoNode();
    int chain();
    void setVideoNode(VideoNode *videoNode);
    void setChain(int chain);

    QImage render();

signals:
    void videoNodeChanged(VideoNode *videoNode);
    void chainChanged(int chain);

private:
    int m_chain;
    VideoNode *m_videoNode;
    QSize m_size;
    QOpenGLFramebufferObject * m_fbo;
    QOpenGLTextureBlitter m_blitter;
    //QSharedPointer<QSGTexture> sgTexture;

protected:
    //QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    //QQuickWindow *m_window;

protected slots:
    //void onWindowChanged(QQuickWindow *window);
};
