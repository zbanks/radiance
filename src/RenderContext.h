#pragma once

#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QVector>
#include <QQuickWindow>
#include <QMutex>

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
    RenderTrigger& operator=(const RenderTrigger&);
public slots:
    void render();
private:
    RenderContext *m_context;
    int m_chain;
    Model *m_model;
    QObject *m_obj; // Object is only needed for equality check in removeRenderTrigger
};

class RenderContextOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    RenderContextOpenGLWorker(RenderContext *p);
public slots:
    void initialize();
signals:
    void initialized();
protected:
    RenderContext *m_p;
};

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
   ~RenderContext() override;
    int chainCount();
    QSize chainSize(int chain);
    GLuint noiseTexture(int chain);
    GLuint blankTexture();
    void makeCurrent();

    // Called from the OpenGLWorkerContext,
    void initialize();

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

protected slots:
    void onInitialized();

private:
    void createNoiseTextures();
    void createBlankTexture();
    void createOpenGLContext();
    QList<int> topoSort(const ModelGraph &graph);
    bool m_initialized;
    QVector<QSharedPointer<QOpenGLTexture> > m_noiseTextures;
    QOpenGLTexture m_blankTexture;
    QList<RenderTrigger> m_renderTriggers;
    RenderContextOpenGLWorker m_openGLWorker;
};
