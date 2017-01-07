#pragma once

#include <QThread>
#include "RenderContext.h"

class RenderThread : public QThread {
    Q_OBJECT

public:
    RenderThread(QObject *p = nullptr);
   ~RenderThread() override;
   void run() override;
signals:
    void render();
    void renderingFinished();
};
