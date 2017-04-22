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
    , m_currentSyncSource(NULL)
    , m_rendering(2)
    , m_noiseTextures(m_outputCount)
{
    connect(this, &RenderContext::addVideoNodeRequested, this, &RenderContext::addVideoNode, Qt::QueuedConnection);
    connect(this, &RenderContext::removeVideoNodeRequested, this, &RenderContext::removeVideoNode, Qt::QueuedConnection);
    connect(this, &RenderContext::renderRequested, this, &RenderContext::render, Qt::QueuedConnection);
}

RenderContext::~RenderContext() {
    delete surface;
    surface = 0;
    delete context;
    context = 0;
    delete m_premultiply;
    m_premultiply = 0;
    foreach(auto t, m_noiseTextures) delete t;
    m_noiseTextures.clear();
}

void RenderContext::start() {
    qDebug() << "Calling start from" << QThread::currentThread();
    context = new QOpenGLContext(this);
    auto scontext = QOpenGLContext::globalShareContext();
    if(scontext) {
        context->setFormat(scontext->format());
        context->setShareContext(scontext);
    }

    context->create();

    // Creating a QOffscreenSurface with no window
    // may fail on some platforms
    // (e.g. wayland)
    surface = new QOffscreenSurface();
    surface->setFormat(context->format());
    surface->create();

    elapsed_timer.start();
}

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

    for(int i=0; i<m_outputCount; i++) {
        auto tex = m_noiseTextures.at(i);
        delete tex;
        tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
        tex->setSize(fboSize(i).width(), fboSize(i).height());
        tex->setFormat(QOpenGLTexture::RGBA8_UNorm);
        tex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        tex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        tex->setWrapMode(QOpenGLTexture::ClampToEdge);

        size_t byteCount = fboSize(i).width() * fboSize(i).height() * 4;
        uint8_t *data = new uint8_t[byteCount];
        qsrand(1);
        for(size_t j=0; j<byteCount; j++) {
            data[j] = qrand() % 256;
        }
        tex->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, data);
        m_noiseTextures[i] = tex;
    }
}

QOpenGLTexture *RenderContext::noiseTexture(int i) {
    return m_noiseTextures.at(i);
}

void RenderContext::update() {
    if(m_rendering.tryAcquire())
    {
        emit renderRequested();
    }
}

void RenderContext::render() {
    qint64 framePeriod = elapsed_timer.nsecsElapsed();
    elapsed_timer.restart();
    {
        QMutexLocker locker(&m_contextLock);

        makeCurrent();
        if(!m_premultiply) load();

        for(auto n : topoSort()) {
            n->render();
        }
    }
    emit renderingFinished();
    qint64 renderingPeriod = elapsed_timer.nsecsElapsed();
    //qDebug() << (1000000000/framePeriod) << renderingPeriod;
    m_rendering.release();
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

void RenderContext::addSyncSource(QObject *source) {
    m_syncSources.append(source);
    if(m_syncSources.last() != m_currentSyncSource) {
        if(m_currentSyncSource != NULL) disconnect(m_currentSyncSource, SIGNAL(frameSwapped()), this, SLOT(update()));
        m_currentSyncSource = m_syncSources.last();
        connect(m_currentSyncSource, SIGNAL(frameSwapped()), this, SLOT(update()), Qt::DirectConnection);
    }
}

void RenderContext::removeSyncSource(QObject *source) {
    m_syncSources.removeOne(source);
    if(m_syncSources.isEmpty()) {
        disconnect(m_currentSyncSource, SIGNAL(frameSwapped()), this, SLOT(update()));
        m_currentSyncSource = NULL;
        qDebug() << "Removed last sync source, video output will stop now";
    }
    else if(m_syncSources.last() != m_currentSyncSource) {
        disconnect(m_currentSyncSource, SIGNAL(frameSwapped()), this, SLOT(update()));
        m_currentSyncSource = m_syncSources.last();
        connect(m_currentSyncSource, SIGNAL(frameSwapped()), this, SLOT(update()), Qt::DirectConnection);
    }
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
