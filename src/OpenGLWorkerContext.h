#pragma once

#include <QThread>
#include <QSharedPointer>
#include <QSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

class OpenGLWorkerContext : public QObject, public QEnableSharedFromThis<OpenGLWorkerContext>, protected QOpenGLFunctions {
public:
    OpenGLWorkerContext(bool threaded=true, QSurface *surface=nullptr);
   ~OpenGLWorkerContext() override;
    QOpenGLContext *context();
    QOpenGLFunctions *glFuncs();
    static QSharedPointer<OpenGLWorkerContext> create(bool threaded = true, QSurface *surface = nullptr);
protected slots:
    void initialize();
    void deinitialize();
public slots:
    void makeCurrent();
    void takeObject(QObject *obj, bool parent=false);
protected slots:
    void onDestroyed();
protected:
    QSurface       *m_surface{};
    QOpenGLContext *m_context{};
    QThread        *m_thread{};
signals:
    void initialized();
};

using OpenGLWorkerContextPointer = QSharedPointer<OpenGLWorkerContext>;
