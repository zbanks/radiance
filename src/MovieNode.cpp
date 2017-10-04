#include "MovieNode.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "main.h"

MovieNode::MovieNode()
    : m_openGLWorker(new MovieNodeOpenGLWorker(this))
    , m_ready(false) {

    connect(m_openGLWorker.data(), &MovieNodeOpenGLWorker::initialized, this, &MovieNode::onInitialized);

    m_mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!m_mpv)
        throw std::runtime_error("could not create mpv context");

    mpv_set_option_string(m_mpv, "terminal", "yes");
    mpv_set_option_string(m_mpv, "msg-level", "all=v");
    mpv_set_option_string(m_mpv, "video-sync", "display");
    mpv_set_option_string(m_mpv, "display-fps", "60");
    if (mpv_initialize(m_mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    // Make use of the MPV_SUB_API_OPENGL_CB API.
    mpv::qt::set_option_variant(m_mpv, "vo", "opengl-cb");

    m_mpv_gl = (mpv_opengl_cb_context *)mpv_get_sub_api(m_mpv, MPV_SUB_API_OPENGL_CB);
    if (!m_mpv_gl)
        throw std::runtime_error("OpenGL not compiled in");

    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);

    bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize");
    Q_ASSERT(result);
}

MovieNode::MovieNode(const MovieNode &other)
    : VideoNode(other)
    , m_openGLWorker(other.m_openGLWorker)
    , m_ready(other.m_ready)
    , m_mpv(other.m_mpv)
    , m_mpv_gl(other.m_mpv_gl) {

    auto k = other.m_renderFbos.keys();
    for (int i=0; i<k.count(); i++) {
        auto otherRenderFbo = other.m_renderFbos.value(k.at(i));
        m_renderFbos.insert(k.at(i), QSharedPointer<QOpenGLFramebufferObject>(otherRenderFbo));
    }
}

MovieNode::~MovieNode() {
}

void MovieNode::command(const QVariant &params) {
    mpv::qt::command_variant(m_mpv, params);
}

void MovieNode::onInitialized() {
    m_ready = true;
}

void MovieNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
    QMutexLocker locker(&m_stateLock);
    for (int i=0; i<added.count(); i++) {
        m_renderFbos.insert(added.at(i), QSharedPointer<QOpenGLFramebufferObject>());
    }
    for (int i=0; i<removed.count(); i++) {
        m_renderFbos.remove(removed.at(i));
    }
}

QString MovieNode::videoPath() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_videoPath;
}

void MovieNode::setVideoPath(QString videoPath) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(videoPath != m_videoPath) {
        {
            QMutexLocker locker(&m_stateLock);
            m_videoPath = videoPath;
        }

        if (m_ready) {
            bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "loadVideo");
            Q_ASSERT(result);
        } else {
            // If not ready, the video will be loaded by the OpenGLWorker's initialize
            qDebug() << "Node not yet ready, waiting for initialize before loading video";
        }

        emit videoPathChanged(videoPath);
    }
}

// See comments in MovieNode.h about these 3 functions
QSharedPointer<VideoNode> MovieNode::createCopyForRendering() {
    return QSharedPointer<VideoNode>(new MovieNode(*this));
}

static void *get_proc_address(void *ctx, const char *name) {
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return NULL;
    return (void *)glctx->getProcAddress(QByteArray(name));
}

GLuint MovieNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    // Hitting this assert means
    // that you failed to make a copy
    // of the VideoNode
    // before rendering in a different thread
    Q_ASSERT(QThread::currentThread() == thread());

    GLuint outTexture = 0;

    if (!m_ready) {
        qDebug() << this << "is not ready";
        return outTexture;
    }
    if (!m_renderFbos.contains(chain)) {
        qDebug() << this << "does not have chain" << chain;
        return outTexture;
    }
    auto renderFbo = m_renderFbos.value(chain);

    // FBO creation must happen here, and not in initialize,
    // because FBOs are not shared among contexts.
    // Textures are, however, so in the future maybe we can move
    // texture creation to initialize()
    // and leave lightweight FBO creation here
    if(renderFbo.isNull()) {
        m_renderFbos[chain] = QSharedPointer<QOpenGLFramebufferObject>(new QOpenGLFramebufferObject(chain->size()));
        renderFbo = m_renderFbos.value(chain);
    }

    mpv_opengl_cb_draw(m_mpv_gl, renderFbo->handle(), chain->size().width(), -chain->size().height());
    outTexture = renderFbo->texture();

    return outTexture;
}

void MovieNode::copyBackRenderState(QSharedPointer<Chain> chain, QSharedPointer<VideoNode> copy) {
    QSharedPointer<MovieNode> c = qSharedPointerCast<MovieNode>(copy);
    QMutexLocker locker(&m_stateLock);
    if (m_renderFbos.contains(chain)) {
        m_renderFbos[chain] = c->m_renderFbos.value(chain);
    } else {
        qDebug() << "Chain was deleted during rendering";
    }
}

// MovieNodeOpenGLWorker methods

MovieNodeOpenGLWorker::MovieNodeOpenGLWorker(MovieNode *p)
    : OpenGLWorker(openGLWorkerContext)
    , m_p(p) {
    connect(this, &MovieNodeOpenGLWorker::message, m_p, &MovieNode::message);
    connect(this, &MovieNodeOpenGLWorker::warning, m_p, &MovieNode::warning);
    connect(this, &MovieNodeOpenGLWorker::fatal,   m_p, &MovieNode::fatal);
    connect(this, &QObject::destroyed, this, &MovieNodeOpenGLWorker::onDestroyed);
}

void MovieNodeOpenGLWorker::initialize() {
    //QMutexLocker locker(&m_p->m_stateLock);

    int r = mpv_opengl_cb_init_gl(m_p->m_mpv_gl, NULL, get_proc_address, NULL);
    if (r < 0)
        throw std::runtime_error("could not initialize OpenGL");

    loadVideo();

    emit initialized();
}

// Call this to load an video into m_frameTextures
// Returns true if the program was loaded successfully
void MovieNodeOpenGLWorker::loadVideo() {
    qDebug() << "LOAD" << m_p->m_videoPath;
    QString filename;
    {
        if (m_p->m_videoPath.isEmpty()) return;
        QMutexLocker locker(&m_p->m_stateLock);
        filename = QString("../resources/videos/%1").arg(m_p->m_videoPath);
    }

    QFileInfo check_file(filename);
    if(!(check_file.exists() && check_file.isFile())) {
        qWarning() << "Could not find" << filename;
        emit warning(QString("Could not find %1").arg(filename));
    }

    m_p->command(QStringList() << "loadfile" << filename);

    qDebug() << "Successfully loaded video" << filename;
}

void MovieNodeOpenGLWorker::onDestroyed() {
    // TODO delete FBOs here
}
