#pragma once

#include <functional>
#include <utility>
#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>
#include <deque>
#include <array>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QImage>
#include <QColor>
#include <QMutex>
#include <QSet>

class RenderContextOld;

class VideoNodeOld : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    VideoNodeOld(RenderContextOld *context);
   ~VideoNodeOld() override;
    std::vector<std::shared_ptr<QOpenGLFramebufferObject> > m_fbos;
    std::vector<std::shared_ptr<QOpenGLFramebufferObject> > m_displayFbos;
    std::vector<std::shared_ptr<QOpenGLFramebufferObject> > m_renderFbos;
    virtual QSet<VideoNodeOld*> dependencies();
    QVector<QColor> pixels(int i, QVector<QPointF>);
    RenderContextOld *context();

/*    virtual QOpenGLFramebufferObject *outputFbo (int idx) const;
    virtual QOpenGLFramebufferObject *displayFbo(int idx) const;
    virtual QOpenGLFramebufferObject *renderFbo (int idx) const;*/

    virtual std::shared_ptr<QOpenGLFramebufferObject> &outputFbo(int idx);
    virtual std::shared_ptr<QOpenGLFramebufferObject> &displayFbo(int idx);
    virtual std::shared_ptr<QOpenGLFramebufferObject> &renderFbo(int idx);

    virtual std::shared_ptr<QOpenGLFramebufferObject> outputFbo(int idx) const;
    virtual std::shared_ptr<QOpenGLFramebufferObject> displayFbo(int idx) const;
    virtual std::shared_ptr<QOpenGLFramebufferObject> renderFbo(int idx) const;

    void resizeFbo(QOpenGLFramebufferObject *&fbo, QSize size);
    template<class Pointer>
    void resizeFbo(Pointer &fbo, QSize size)
    {
        if(!fbo || fbo->size() != size) {
//            auto raw = fbo.release();
//            resizeFbo(raw, size);
//            fbo.reset(raw);
            auto rep = Pointer(new QOpenGLFramebufferObject(size));
            auto tex = rep->texture();
            glBindTexture(GL_TEXTURE_2D,tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
            using std::swap;
            swap(fbo, rep);
        }
    }

public slots:
    void render();
    bool swap(int i);

protected:
    virtual void initialize() = 0;
    virtual void paint() = 0;
    void blitToRenderFbo();
    RenderContextOld *m_context;

    void beforeDestruction();

signals:
    void initialized();

private:
    std::vector<QMutex> m_textureLocks;
    std::unique_ptr<std::atomic<bool>[]> m_updated;
    bool m_initialized;

    QMutex m_previewImageLock;
    QImage m_previewImage;
    bool m_previewImageValid;
};
