#pragma once

#include <QObject>
#include "OpenGLWorkerContext.h"

class OpenGLWorker : public QObject {
    Q_OBJECT

public:
    OpenGLWorker(OpenGLWorkerContext *m_context);
   ~OpenGLWorker() override;
    void makeCurrent();
    QOpenGLContext *openGLContext();
    QOpenGLFunctions *glFuncs();
private:
    OpenGLWorkerContext *m_context;
};
