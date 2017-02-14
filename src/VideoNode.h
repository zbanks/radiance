#pragma once

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QImage>
#include <QColor>
#include <QMutex>
#include <QSet>

class RenderContext;

class VideoNode : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    VideoNode(RenderContext *context);
   ~VideoNode() override;
    QVector<QOpenGLFramebufferObject *> m_fbos;
    QVector<QOpenGLFramebufferObject *> m_displayFbos;
    QVector<QOpenGLFramebufferObject *> m_renderFbos;
    virtual QSet<VideoNode*> dependencies();
    QVector<QColor> pixels(int i, QVector<QPointF>);
    RenderContext *context();

public slots:
    void render();
    bool swap(int i);

protected:
    virtual void initialize() = 0;
    virtual void paint() = 0;
    void blitToRenderFbo();
    RenderContext *m_context;
    static void resizeFbo(QOpenGLFramebufferObject **fbo, QSize size);
    void beforeDestruction();

signals:
    void initialized();

private:
    QVector<QMutex *> m_textureLocks;
    QVector<bool> m_updated;
    bool m_initialized;

    QMutex m_previewImageLock;
    QImage m_previewImage;
    bool m_previewImageValid;
};
