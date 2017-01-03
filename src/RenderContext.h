#pragma once

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QTimer>

class RenderContext : public QObject {
    Q_OBJECT
public:
    RenderContext();
    QOffscreenSurface *surface;
    QOpenGLContext *context;
    QTimer *timer;

    //void makeContext(QOpenGLContext *current);
    void share(QOpenGLContext *current);
    void makeCurrent();
    void flush();
public slots:
    void start();
    void finish();
private slots:
    void tick();
};
