#include "RenderThread.h"
#include "RenderContext.h"
#include "main.h"

RenderThread::RenderThread(QObject *p)
: QThread(p)
{
    setObjectName("RenderThread");
}
RenderThread::~RenderThread()
{
    quit();
    wait();
}
void RenderThread::run()
{
    RenderContext rc{};
    connect(this, &RenderThread::render, &rc,&RenderContext::renderDirect, Qt::DirectConnection);
    connect(this, &RenderThread::render, &rc,&RenderContext::render);
    connect(&rc,&RenderContext::renderingFinished, this,&RenderThread::renderingFinished);
    renderContext = &rc;
    rc.start();
    QThread::exec();
    renderContext = nullptr;
}
