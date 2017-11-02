#pragma once

#include <QThread>
#include <QSharedPointer>
#include <QSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

class OpenGLWorkerContext : public QObject, protected QOpenGLFunctions {
public:
    OpenGLWorkerContext(QObject *p, bool threaded=true, QSurface *surface=nullptr);
    OpenGLWorkerContext(bool threaded=true, QSurface *surface=nullptr);
    ~OpenGLWorkerContext();
    QOpenGLContext *context();
    QOpenGLFunctions *glFuncs();

protected slots:
    void initialize();
    void deinitialize();
public slots:
    void makeCurrent();
    void takeObject(QObject *obj);
protected slots:
    void onDestroyed();
protected:
    QSurface       *m_surface{};
    QOpenGLContext *m_context{};
    QThread        *m_thread{};
};
