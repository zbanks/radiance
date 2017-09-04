#pragma once

#include <QThread>
#include <QSharedPointer>
#include <QSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

class OpenGLWorkerContext : public QObject, protected QOpenGLFunctions {
public:
    OpenGLWorkerContext(bool threaded = true, QSharedPointer<QSurface> surface = QSharedPointer<QSurface>());
    ~OpenGLWorkerContext();
    QOpenGLContext *context();
    QOpenGLFunctions *glFuncs();

protected slots:
    void initialize();
public slots:
    void makeCurrent();
protected:
    QSharedPointer<QSurface> m_surface;
    QOpenGLContext *m_context;
private:
    bool m_threaded;
};
