#pragma once

#include <QThread>
#include <QSharedPointer>
#include <QSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

// I know you aren't supposed to subclass QThread
// but I think I know what I'm doing
class OpenGLWorkerContext
    : public QThread
    , protected QOpenGLFunctions {
public:
    OpenGLWorkerContext(QSharedPointer<QSurface> surface = QSharedPointer<QSurface>());
    ~OpenGLWorkerContext() override;
    QOpenGLContext *context();
    QOpenGLFunctions *glFuncs();
protected slots:
    void initialize();
public slots:
    void makeCurrent();
protected:
    QSharedPointer<QSurface> m_surface;
    QSharedPointer<QOpenGLContext> m_context;
    QSharedPointer<QOpenGLFunctions> m_glFuncs;
};
