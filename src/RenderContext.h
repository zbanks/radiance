#pragma once

#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QVector>
#include <QQuickWindow>
#include <QMutex>
#include <QTimer>
#include "RenderTrigger.h"

// The Everpresent God Object (EGO)

typedef int VnId;

class VideoNode;
class Model;
class RenderContext;

class RenderContextOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    RenderContextOpenGLWorker(RenderContext* p);
public slots:
    void initialize();
signals:
    void initialized();
protected:
    void createBlankTexture();
    void createNoiseTextures();
    RenderContext *m_p;
};

///////////////////////////////////////////////////////////////////////////////

class RenderContext : public QObject {
    Q_OBJECT

    friend class RenderContextOpenGLWorker;

public:
    RenderContext();
   ~RenderContext() override;
    int chainCount();
    QSize chainSize(int chain);
    GLuint noiseTexture(int chain);
    GLuint blankTexture();
    void makeCurrent();

    static constexpr int PERIODIC_MS = 10;

public slots:
    QMap<VnId, GLuint> render(Model *m, int chain);

    // This is an annoying little hack
    // to get around QML's difficulty
    // in making a DirectConnection from
    // beforeSynchronizing to render
    void addRenderTrigger(QQuickWindow *window, Model *model, int chain);
    void removeRenderTrigger(QQuickWindow *window, Model *model, int chain);
    // In the future we can override this function so that
    // more than just QQuickWindows can trigger renders

    // Returns a unique ID
    // (just a sequence number)
    VnId registerVideoNode();

    // Returns the texture from the last render
    // Used as a hack to pass textures
    // into the scene graph
    // where they are set in updatePaintNode
    GLuint lastRender(int chain, VnId videoNodeId); // XXX dirty hack

signals:
    // This signal is fired periodically
    // to tell all nodes to update.
    // Since rendering happens asynchronously,
    // nodes should use this signal
    // to increment counters, integrate values, etc.
    void periodic();

protected slots:
    void onInitialized();

private:
    bool m_initialized;
    QVector<QSharedPointer<QOpenGLTexture>> m_noiseTextures;
    QOpenGLTexture m_blankTexture;
    QList<RenderTrigger> m_renderTriggers;
    RenderContextOpenGLWorker m_openGLWorker;
    QTimer m_periodic;
    VnId m_vnId;
    QVector<QMap<VnId, GLuint>> m_lastRender; // XXX dirty hack
};
