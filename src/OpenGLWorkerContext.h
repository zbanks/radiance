#pragma once

#include <QThread>
#include <QSharedPointer>
#include <QSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

class OpenGLWorkerContext : public QObject, protected QOpenGLFunctions {
public:
    OpenGLWorkerContext(bool threaded=true, QSurface *surface=nullptr);
    ~OpenGLWorkerContext();
    QOpenGLContext *context();
    QOpenGLFunctions *glFuncs();

protected slots:
    void initialize();
public slots:
    void makeCurrent();
protected:
    QSurface *m_surface;
    QOpenGLContext *m_context;
private:
    bool m_threaded;
};
