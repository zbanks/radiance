#pragma once

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QEvent>
#include <QSet>
#include "VideoNode.h"

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
    ~RenderContext();
    QOffscreenSurface *surface;
    QOpenGLContext *context;
    QTimer *timer;
    QElapsedTimer elapsed_timer;
    QMutex m_contextLock;
    void moveToThread(QThread *t);

    void makeCurrent();
    void flush();
    void share(QOpenGLContext *current);
    void addVideoNode(VideoNode* n);
    void removeVideoNode(VideoNode* n);
 
public slots:
    void start();
    void render();

private:
    QSet<VideoNode*> m_videoNodes;

signals:
    void renderingFinished();
};
