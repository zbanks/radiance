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
    , m_blankFbo()
    , m_framePeriodLPF(0)
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
//    delete m_blankFbo;
//    m_blankFbo = 0;
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

void RenderContext::checkLoadShaders() {
    if(m_premultiply != nullptr) return;

    auto program = m_premultiply;
    program = new QOpenGLShaderProgram(this);
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

QOpenGLTexture *RenderContext::noiseTexture(int i) {
    return m_noiseTextures.at(i);
}

std::shared_ptr<QOpenGLFramebufferObject> &RenderContext::blankFbo() {
    return m_blankFbo;
}

void RenderContext::update() {
    if(m_rendering.tryAcquire()) {
        emit renderRequested();
    }
}

void RenderContext::checkCreateNoise() {
    for(int i=0; i<m_outputCount; i++) {
        auto tex = m_noiseTextures.at(i);
        if(tex &&
           tex->width() == fboSize(i).width() &&
           tex->height() == fboSize(i).height()) {
            continue;
        }
        delete tex;
        tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
        tex->setSize(fboSize(i).width(), fboSize(i).height());
        tex->setFormat(QOpenGLTexture::RGBA8_UNorm);
        tex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        tex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        tex->setWrapMode(QOpenGLTexture::Repeat);

        auto byteCount = fboSize(i).width() * fboSize(i).height() * 4;
        auto data = std::make_unique<uint8_t[]>(byteCount);
        qsrand(1);
        std::generate(&data[0],&data[0] + byteCount,qrand);
        tex->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
        m_noiseTextures[i] = tex;
    }
}

void RenderContext::checkCreateBlankFbo()
{
    if(!m_blankFbo) {
        m_blankFbo = std::make_shared<QOpenGLFramebufferObject>(QSize(1,1));
        glBindTexture(GL_TEXTURE_2D, m_blankFbo->texture());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void RenderContext::render() {
    qint64 framePeriod = elapsed_timer.nsecsElapsed();
    elapsed_timer.restart();
    {
        QMutexLocker locker(&m_contextLock);

        makeCurrent();
        checkLoadShaders();
        checkCreateNoise();
        checkCreateBlankFbo();

        for(auto n : topoSort()) {
            n->render();
        }
    }
    emit renderingFinished();
    //qint64 renderingPeriod = elapsed_timer.nsecsElapsed();
    m_framePeriodLPF += FPS_ALPHA * (framePeriod - m_framePeriodLPF);
    m_rendering.release();
    emit fpsChanged(fps());
}

qreal RenderContext::fps() {
    return 1000000000/m_framePeriodLPF;
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

    auto sortedNodes = QList<VideoNode*>{};
    auto fwdEdges = std::map<VideoNode*, QSet<VideoNode*> >{};
    auto revEdges = std::map<VideoNode*, int>{};

    auto startNodes = std::deque<VideoNode*>{};
    auto videoNodes = m_videoNodes;
    for(auto && n: videoNodes) {
        auto deps = n->dependencies();
        revEdges.emplace(n, deps.size());
        if(deps.empty())
            startNodes.push_back(n);
        else for(auto c : deps)
            fwdEdges[c].insert(n);

    }
    while(!startNodes.empty()) {
        auto n = startNodes.back();
        startNodes.pop_back();
        sortedNodes.append(n);
        auto fwd_it = fwdEdges.find(n);
        if(fwd_it != fwdEdges.end()) {
            for(auto c: fwd_it->second) {
                auto &refcnt = revEdges[c];
                if(!--refcnt)
                    startNodes.push_back(c);
            }
            fwdEdges.erase(fwd_it);
        }
    }
    if(!fwdEdges.empty()) {
        qDebug() << "Cycle detected!";
        return {};
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
    if(i == previewFboIndex())
        return uiSettings->previewSize();
    if(i == outputFboIndex())
        return uiSettings->outputSize();
    return QSize(0, 0);
}
