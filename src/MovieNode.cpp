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
    connect(this, &MovieNode::videoPathChanged, m_openGLWorker.data(), &MovieNodeOpenGLWorker::onVideoChanged);

    bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize");
    Q_ASSERT(result);
}

MovieNode::MovieNode(const MovieNode &other)
    : VideoNode(other)
    , m_openGLWorker(other.m_openGLWorker)
    , m_ready(other.m_ready)
    , m_blitShader(other.m_blitShader) {

    auto k = other.m_renderFbos.keys();
    for (int i=0; i<k.count(); i++) {
        auto otherRenderFbo = other.m_renderFbos.value(k.at(i));
        m_renderFbos.insert(k.at(i), QSharedPointer<QOpenGLFramebufferObject>(otherRenderFbo));
    }
}

MovieNode::~MovieNode() {
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

    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    renderFbo->bind();
    m_blitShader->bind();
    glActiveTexture(GL_TEXTURE0);
    {
        int i = m_openGLWorker->lastIndex();
        QMutexLocker locker(m_openGLWorker->m_fboLocks.at(i));
        if (m_openGLWorker->m_fbos.at(i) == nullptr) return outTexture;
        glBindTexture(GL_TEXTURE_2D, m_openGLWorker->m_fbos.at(i)->texture());
        m_blitShader->setUniformValue("iVideoFrame", 0);
        m_blitShader->setUniformValue("iResolution", GLfloat(chain->size().width()), GLfloat(chain->size().height()));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glFlush();
    }
    m_blitShader->release();
    renderFbo->release();
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
    , m_p(p)
    , m_fbos(m_bufferCount)
    , m_fboLocks(m_bufferCount)
    , m_fboIndex(0) {

    for (int i=0; i<m_bufferCount; i++) {
        m_fboLocks[i] = new QMutex();
    }

    connect(this, &MovieNodeOpenGLWorker::message, m_p, &MovieNode::message);
    connect(this, &MovieNodeOpenGLWorker::warning, m_p, &MovieNode::warning);
    connect(this, &MovieNodeOpenGLWorker::fatal,   m_p, &MovieNode::fatal);
    connect(this, &QObject::destroyed, this, &MovieNodeOpenGLWorker::onDestroyed);
}

static void requestUpdate(void *ctx) {
    QMetaObject::invokeMethod((MovieNodeOpenGLWorker*)ctx, "drawFrame");
}

void MovieNodeOpenGLWorker::initialize() {
    setlocale(LC_NUMERIC, "C");
    m_mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!m_mpv)
        throw std::runtime_error("could not create mpv context");

    mpv_set_option_string(m_mpv, "terminal", "yes");
    mpv_set_option_string(m_mpv, "msg-level", "all=v");
    if (mpv_initialize(m_mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    //mpv_set_property_string(m_mpv, "video-sync", "display-resample");
    //mpv_set_property_string(m_mpv, "display-fps", "60");
    mpv_set_property_string(m_mpv, "hwdec", "auto");
    mpv_set_property_string(m_mpv, "loop", "inf");

    // Make use of the MPV_SUB_API_OPENGL_CB API.
    mpv::qt::set_option_variant(m_mpv, "vo", "opengl-cb");

    m_mpv_gl = (mpv_opengl_cb_context *)mpv_get_sub_api(m_mpv, MPV_SUB_API_OPENGL_CB);
    if (!m_mpv_gl)
        throw std::runtime_error("OpenGL not compiled in");

    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);

    int r = mpv_opengl_cb_init_gl(m_mpv_gl, NULL, get_proc_address, NULL);
    if (r < 0)
        throw std::runtime_error("could not initialize OpenGL");

    mpv_opengl_cb_set_update_callback(m_mpv_gl, requestUpdate, (void *)this);

    if (!loadBlitShader()) return;

    emit initialized();

    onVideoChanged();
}

void MovieNodeOpenGLWorker::drawFrame() {
    {
        QMutexLocker locker(m_fboLocks[m_fboIndex]);
        qint64 videoWidth;
        qint64 videoHeight;
        if (mpv_get_property(m_mpv, "video-params/w", MPV_FORMAT_INT64, &videoWidth) < 0)
            throw std::runtime_error("could not get video width");
        if (mpv_get_property(m_mpv, "video-params/h", MPV_FORMAT_INT64, &videoHeight) < 0)
            throw std::runtime_error("could not get video height");
        QSize size = QSize(videoWidth, videoHeight);
        if (m_fbos.at(m_fboIndex) == nullptr || m_fbos.at(m_fboIndex)->size() != size) {
            delete m_fbos.at(m_fboIndex);
            m_fbos[m_fboIndex] = new QOpenGLFramebufferObject(size);
        }
        auto fbo = m_fbos.at(m_fboIndex);

        mpv_opengl_cb_draw(m_mpv_gl, fbo->handle(), fbo->width(), -fbo->height());
        glFlush();
    }
    m_fboIndex = (m_fboIndex + 1) % m_bufferCount;
}

int MovieNodeOpenGLWorker::lastIndex() {
    return (m_fboIndex + m_bufferCount - 1) % m_bufferCount;
}

void MovieNodeOpenGLWorker::onVideoChanged() {
    qDebug() << "LOAD" << m_p->m_videoPath;
    if (!m_mpv_gl) return; // Wait for initialization
    QString filename;
    {
        QMutexLocker locker(&m_p->m_stateLock);
        if (m_p->m_videoPath.isEmpty()) return;
        filename = QString("../resources/videos/%1").arg(m_p->m_videoPath);
    }

    QFileInfo check_file(filename);
    if(!(check_file.exists() && check_file.isFile())) {
        qWarning() << "Could not find" << filename;
        emit warning(QString("Could not find %1").arg(filename));
    }

    command(QStringList() << "loadfile" << filename);

    qDebug() << "Successfully loaded video" << filename;
}

void MovieNodeOpenGLWorker::command(const QVariant &params) {
    mpv::qt::command_variant(m_mpv, params);
}

void MovieNodeOpenGLWorker::onDestroyed() {
    qDeleteAll(m_fbos.begin(), m_fbos.end());
    qDeleteAll(m_fboLocks.begin(), m_fboLocks.end());
}

bool MovieNodeOpenGLWorker::loadBlitShader() {
    auto vertexString = QString{
        "#version 130\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "const vec2 varray[4] = { vec2( 1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.)};\n"
        "out vec2 coords;\n"
        "void main() {\n"
        "    vec2 vertex = varray[gl_VertexID];\n"
        "    gl_Position = vec4(vertex,0.,1.);\n"
        "    coords = vertex;\n"
        "}\n"};
    auto fragmentString = QString{
        "#version 130\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "uniform sampler2D iVideoFrame;\n"
        "uniform vec2 iResolution;\n"
        "vec2 uv = gl_FragCoord.xy / iResolution;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(iVideoFrame, uv);\n"
        "}\n"};

    m_p->m_blitShader = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());
    if(!m_p->m_blitShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
        emit fatal("Could not compile vertex shader");
        return false;
    }
    if(!m_p->m_blitShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
        emit fatal("Could not compile fragment shader");
        return false;
    }
    if(!m_p->m_blitShader->link()) {
        emit fatal("Could not link shader program");
        return false;
    }
    qDebug() << "Loaded blit shader";
    return true;
}
