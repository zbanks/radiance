#pragma once

#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QVector>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QQuickWindow>

// The Everpresent God Object (EGO)

class VideoNode;
class Model;
class RenderContext;
class ModelGraph;

class RenderTrigger : public QObject {
    Q_OBJECT

public:
    RenderTrigger(RenderContext *context, Model *model, int chain, QObject *obj);
    RenderTrigger(const RenderTrigger&);
   ~RenderTrigger();
    bool operator==(const RenderTrigger &other) const;
public slots:
    void render();
private:
    RenderContext *m_context;
    int m_chain;
    Model *m_model;
    QObject *m_obj; // Object is only needed for equality check in removeRenderTrigger
};

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
   ~RenderContext() override;
    void initialize();
    int chainCount();
    QSize chainSize(int chain);
    QSharedPointer<QOpenGLTexture> noiseTexture(int chain);
    QSharedPointer<QOpenGLTexture> blankTexture();
    void makeCurrent();

public slots:
    void render(Model *m, int chain);

    // This is an annoying little hack
    // to get around QML's difficulty
    // in making a DirectConnection from
    // beforeSynchronizing to render
    void addRenderTrigger(QQuickWindow *window, Model *model, int chain);
    void removeRenderTrigger(QQuickWindow *window, Model *model, int chain);
    // In the future we can override this function so that
    // more than just QQuickWindows can trigger renders

private:
    void createNoiseTextures();
    void createBlankTexture();
    void createOpenGLContext();
    QList<int> topoSort(const ModelGraph &graph);
    bool m_initialized;
    QVector<QSharedPointer<QOpenGLTexture> > m_noiseTextures;
    QSharedPointer<QOpenGLTexture> m_blankTexture;
    QSharedPointer<QOpenGLContext> m_context;
    QSharedPointer<QOffscreenSurface> m_surface;
    QList<RenderTrigger> m_renderTriggers;
};
