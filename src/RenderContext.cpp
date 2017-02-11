#include "RenderContext.h"
#include "main.h"
#include <QOpenGLFunctions>
#include <QDebug>
#include <QThread>

RenderContext::RenderContext()
    : context(nullptr)
    , surface(nullptr)
    , timer(nullptr)
    , m_premultiply(nullptr)
    , m_outputCount(2)
{
    context = new QOpenGLContext(this);
    auto scontext = QOpenGLContext::globalShareContext();
    if(scontext) {
        context->setFormat(scontext->format());
        context->setShareContext(scontext);
    }

    context->create();

    surface = new QOffscreenSurface();
    surface->setFormat(context->format());
    surface->create();
    connect(this, &RenderContext::addVideoNodeRequested, this, &RenderContext::addVideoNode, Qt::QueuedConnection);
    connect(this, &RenderContext::removeVideoNodeRequested, this, &RenderContext::removeVideoNode, Qt::QueuedConnection);
}

RenderContext::~RenderContext() {
    delete surface;
    surface = 0;
    delete context;
    context = 0;
    delete m_premultiply;
    m_premultiply = 0;
}

void RenderContext::start() {
    elapsed_timer.start();
}

// Radiance creates its own OpenGL context for rendering,
// in case it is running with no UI. If there is a Qt UI,
// you will need to call this function with the UI context
// to properly set up context sharing.

void RenderContext::load() {
    auto program = m_premultiply;
    m_premultiply = nullptr;
    if(!program)
        program = new QOpenGLShaderProgram(this);
    else
        program->removeAllShaders();
    program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                       "attribute highp vec4 vertices;"
                                       "varying highp vec2 coords;"
                                       "void main() {"
                                       "    gl_Position = vertices;"
                                       "    coords = vertices.xy;"
                                       "}");
    program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                       "varying highp vec2 coords;"
                                       "uniform sampler2D iFrame;"
                                       "void main() {"
                                       "    vec4 l = texture2D(iFrame, 0.5 * (coords + 1.));"
                                       "    gl_FragColor = vec4(l.rgb * l.a, l.a);"
                                       "}");
    program->bindAttributeLocation("vertices", 0);
    program->link();
    m_premultiply = program;
}

void RenderContext::render() {
    qint64 framePeriod = elapsed_timer.nsecsElapsed();
    elapsed_timer.restart();
    {
        QMutexLocker locker(&m_contextLock);
        makeCurrent();

        if(!m_premultiply)
            load();

        for(auto n : topoSort()) {
            n->render();
        }
    }
    emit renderingFinished();
    qint64 renderingPeriod = elapsed_timer.nsecsElapsed();
    //qDebug() << framePeriod << renderingPeriod;
}

void RenderContext::makeCurrent() {
    context->makeCurrent(surface);
}

void RenderContext::flush() {
    context->functions()->glFinish();
}

void RenderContext::addVideoNode(VideoNode* n) {
    // It is less clear to me if taking the context lock
    // is necessary here
    QMutexLocker locker(&m_contextLock);
    m_videoNodes.insert(n);
}

void RenderContext::removeVideoNode(VideoNode* n) {
    // Take the context lock to avoid deleting anything
    // required for the current render
    QMutexLocker locker(&m_contextLock);
    m_videoNodes.remove(n);
}

QList<VideoNode*> RenderContext::topoSort()
{
    // Fuck this

    QList<VideoNode*> sortedNodes;
    QMap<VideoNode*, QSet<VideoNode*> > fwdEdges;
    QMap<VideoNode*, QSet<VideoNode*> > revEdges;

    foreach(VideoNode* n, m_videoNodes) {
        auto children = n->dependencies();
        revEdges.insert(n, children);
        foreach(VideoNode* c, children) {
            fwdEdges[c].insert(n);
        }
    }

    QList<VideoNode*> startNodes;

    foreach(VideoNode* n, m_videoNodes) {
        if(revEdges.value(n).isEmpty()) startNodes.append(n);
    }

    while(!startNodes.isEmpty()) {
        VideoNode* n = startNodes.takeLast();
        sortedNodes.append(n);
        for(auto c: fwdEdges.value(n)) {
            revEdges[c].remove(n);
            if(revEdges.value(c).isEmpty())
                startNodes.append(c);
        }
        fwdEdges.remove(n);
    }

    if(!fwdEdges.isEmpty()) {
        qDebug() << "Cycle detected!";
        return QList<VideoNode*>();
    }

    return sortedNodes;
}

int RenderContext::outputCount() {
    return m_outputCount;
}


int RenderContext::previewFboIndex() {
    return 0;
}

int RenderContext::outputFboIndex() {
    return 1;
}

QSize RenderContext::fboSize(int i) {
    if(i == previewFboIndex()) return uiSettings->previewSize();
    if(i == outputFboIndex()) return uiSettings->outputSize();
    return QSize(0, 0);
}
