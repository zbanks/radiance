#include "MovieNode.h"
#include <QDebug>
#include <QReadWriteLock>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLVertexArrayObject>
#include <locale.h>
#include <array>
#include <QJsonObject>
#include "Paths.h"

MovieNode::MovieNode(Context *context, QString videoPath)
    : VideoNode(context)
    , m_videoPath(videoPath)
    , m_openGLWorker()
    , m_renderFbos()
    , m_blitShader()
    , m_videoSize()
    , m_chainSize()
    , m_ready()
    , m_mute(true)
    , m_pause() {
}

MovieNode::MovieNode(const MovieNode &other)
    : VideoNode(other)
    , m_videoPath(other.m_videoPath)
    , m_openGLWorker(other.m_openGLWorker)
    , m_renderFbos(other.m_renderFbos)
    , m_blitShader(other.m_blitShader)
    , m_videoSize(other.m_videoSize)
    , m_chainSize(other.m_chainSize)
    , m_ready(other.m_ready)
    , m_mute(other.m_mute)
    , m_pause(other.m_pause) {
}

MovieNode::~MovieNode() {
}

QJsonObject MovieNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("videoPath", m_videoPath);
    return o;
}

void MovieNode::onInitialized() {
    {
        QMutexLocker locker(&m_stateLock);
        for (auto key : m_renderFbos.keys()) {
            auto state = QSharedPointer<MovieNodeRenderState>::create();
            m_openGLWorker->prepareState(state);
            m_renderFbos[key].swap(state);
        }
    }
   m_ready = true;
}

void MovieNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
    QMutexLocker locker(&m_stateLock);
    for (int i=0; i<added.count(); i++) {
        auto state = QSharedPointer<MovieNodeRenderState>::create();
        m_openGLWorker->prepareState(state);
        m_renderFbos.insert(added.at(i), state);
    }
    for (int i=0; i<removed.count(); i++) {
        m_renderFbos.remove(removed.at(i));
    }
    auto _size = QSize{};
    for(auto && chain : m_renderFbos.keys()) {
        auto _csize = chain->size();
        _size.setWidth(std::max(_csize.width(),_size.width()));
        _size.setHeight(std::max(_csize.height(),_size.height()));
    }
    if(_size != m_chainSize) {
        m_chainSize = _size;
        emit chainSizeChanged(_size);
    }
}

QString MovieNode::videoPath() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_videoPath;
}

qreal MovieNode::position() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_position;
}

qreal MovieNode::duration() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_duration;
}

QSize MovieNode::videoSize() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_videoSize;
}

bool MovieNode::mute() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_mute;
}

bool MovieNode::pause() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_pause;
}

void MovieNode::onVideoSizeChanged(QSize size) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(size != m_videoSize) {
        {
            QMutexLocker locker(&m_stateLock);
            m_videoSize = size;
        }

        emit videoSizeChanged(size);
    }
}

void MovieNode::onPositionChanged(qreal position) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(position != m_position) {
        {
            QMutexLocker locker(&m_stateLock);
            m_position = position;
        }

        emit positionChanged(position);
    }
}

void MovieNode::onDurationChanged(qreal duration) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(duration != m_duration) {
        {
            QMutexLocker locker(&m_stateLock);
            m_duration = duration;
        }

        emit durationChanged(duration);
    }
}

void MovieNode::onMuteChanged(bool mute) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (mute != m_mute) {
        {
            QMutexLocker locker(&m_stateLock);
            m_mute = mute;
        }
        emit muteChanged(mute);
    }
}

void MovieNode::onPauseChanged(bool pause) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (pause != m_pause) {
        {
            QMutexLocker locker(&m_stateLock);
            m_pause = pause;
        }
        emit pauseChanged(pause);
    }
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

void MovieNode::setPosition(qreal position) {
    Q_ASSERT(QThread::currentThread() == thread());
    QMetaObject::invokeMethod(m_openGLWorker.data(), "setPosition", Qt::QueuedConnection, Q_ARG(qreal, position));
}

void MovieNode::setMute(bool mute) {
    Q_ASSERT(QThread::currentThread() == thread());
    QMetaObject::invokeMethod(m_openGLWorker.data(), "setMute", Qt::QueuedConnection, Q_ARG(bool, mute));
}

void MovieNode::setPause(bool pause) {
    Q_ASSERT(QThread::currentThread() == thread());
    QMetaObject::invokeMethod(m_openGLWorker.data(), "setPause", Qt::QueuedConnection, Q_ARG(bool, pause));
}

// See comments in MovieNode.h about these 3 functions
QSharedPointer<VideoNode> MovieNode::createCopyForRendering(QSharedPointer<Chain> chain) {
    Q_UNUSED(chain);
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
    auto renderState = m_renderFbos.value(chain);

    if(!renderState|| !renderState->m_ready.load())
        return outTexture;

    auto renderFbo = renderState->m_pass.m_output;
    // FBO creation must happen here, and not in initialize,
    // because FBOs are not shared among contexts.
    // Textures are, however, so in the future maybe we can move
    // texture creation to initialize()
    // and leave lightweight FBO creation here
    if(!renderFbo || renderFbo->size() != chain->size()) {
        auto fmt = QOpenGLFramebufferObjectFormat{};
        fmt.setInternalTextureFormat(GL_RGBA);
        renderFbo = renderState->m_pass.m_output = QSharedPointer<QOpenGLFramebufferObject>::create(chain->size(),fmt);
    }

    {
        QMutexLocker locker(&m_openGLWorker->m_rwLock);
        auto fboi = m_openGLWorker->m_frames.front();
        if (fboi) {
            glClearColor(0, 0, 0, 0);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            renderFbo->bind();
            glViewport(0, 0, renderFbo->width(),renderFbo->height());

            auto blitShader = renderState->m_pass.m_shader;
            blitShader->bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fboi->texture());
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

            blitShader->setUniformValue("iVideoFrame", 0);
            blitShader->setUniformValue("iResolution", GLfloat(renderFbo->width()), GLfloat(renderFbo->height()));

            blitShader->setUniformValue("iVideoResolution", GLfloat(fboi->width()), GLfloat(fboi->height()));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            renderFbo->release();
            outTexture = renderFbo->texture();

            blitShader->release();

        }
    }

    return outTexture;
}

// MovieNodeOpenGLWorker methods

MovieNodeOpenGLWorker::MovieNodeOpenGLWorker(MovieNode *p)
    : OpenGLWorker(p->m_openGLWorkerContext.data())
    , m_p(p)
    , m_mpv_gl(nullptr)
    , m_size(0, 0)
{

    qRegisterMetaType<QSharedPointer<MovieNodeRenderState>>();
    connect(this, &MovieNodeOpenGLWorker::prepareState, this, &MovieNodeOpenGLWorker::onPrepareState);
    connect(this, &MovieNodeOpenGLWorker::message, m_p, &MovieNode::message);
    connect(this, &MovieNodeOpenGLWorker::warning, m_p, &MovieNode::warning);
    connect(this, &MovieNodeOpenGLWorker::fatal,   m_p, &MovieNode::fatal);
}

static void requestUpdate(void *ctx) {
    QMetaObject::invokeMethod((MovieNodeOpenGLWorker*)ctx, "drawFrame", Qt::QueuedConnection);
}

static void requestWakeup(void *ctx) {
    QMetaObject::invokeMethod((MovieNodeOpenGLWorker*)ctx, "onEvent", Qt::QueuedConnection);
}

void MovieNodeOpenGLWorker::initialize() {
    setlocale(LC_NUMERIC, "C");
    m_mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!m_mpv)
        throw std::runtime_error("could not create mpv context");

    mpv_set_option_string(m_mpv, "terminal", "yes");
    mpv_set_option_string(m_mpv, "msg-level", "all=v");
    mpv_set_option_string(m_mpv, "ytdl", "yes");
    if (mpv_initialize(m_mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    //mpv_set_property_string(m_mpv, "video-sync", "display-resample");
    //mpv_set_property_string(m_mpv, "display-fps", "60");
    mpv_set_property_string(m_mpv, "hwdec", "auto");
    mpv_set_property_string(m_mpv, "scale", "spline36");
    mpv_set_property_string(m_mpv, "loop", "inf");
    mpv_set_property_string(m_mpv, "fbo-format", "rgba32f");
    mpv_set_property_string(m_mpv, "opengl-fbo-format", "rgba32f");

    // Make use of the MPV_SUB_API_OPENGL_CB API.
    mpv::qt::set_option_variant(m_mpv, "vo", "opengl-cb");

    m_mpv_gl = (mpv_opengl_cb_context *)mpv_get_sub_api(m_mpv, MPV_SUB_API_OPENGL_CB);
    if (!m_mpv_gl)
        throw std::runtime_error("OpenGL not compiled in");

    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "video-params/w", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, 0, "video-params/h", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);

    int r = mpv_opengl_cb_init_gl(m_mpv_gl, NULL, get_proc_address, NULL);
    if (r < 0)
        throw std::runtime_error("could not initialize OpenGL");

    mpv_opengl_cb_set_update_callback(m_mpv_gl, requestUpdate, (void *)this);

    mpv_set_wakeup_callback(m_mpv, requestWakeup, this);

    if (!loadBlitShader()) return;

    emit initialized();

    onVideoChanged();
}

void MovieNodeOpenGLWorker::onEvent() {
    while (m_mpv) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }
        handleEvent(event);
    }
}

void MovieNodeOpenGLWorker::handleEvent(mpv_event *event) {
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        mpv_event_property *prop = (mpv_event_property *)event->data;
        if (strcmp(prop->name, "time-pos") == 0) {
            if (prop->format == MPV_FORMAT_DOUBLE) {
                double time = *(double *)prop->data;
                emit positionChanged(time);
            }
        } else if (strcmp(prop->name, "duration") == 0) {
            if (prop->format == MPV_FORMAT_DOUBLE) {
                double time = *(double *)prop->data;
                emit durationChanged(time);
            }
        } else if (strcmp(prop->name, "video-params/w") == 0) {
            if (prop->format == MPV_FORMAT_INT64) {
                qint64 d = *(qint64 *)prop->data;
                m_videoSize.setWidth(d);
                updateSizes();
                emit videoSizeChanged(m_videoSize);
            }
        } else if (strcmp(prop->name, "video-params/h") == 0) {
            if (prop->format == MPV_FORMAT_INT64) {
                qint64 d = *(qint64 *)prop->data;
                m_videoSize.setHeight(d);
                updateSizes();
                emit videoSizeChanged(m_videoSize);
            }
        } else if (strcmp(prop->name, "mute") == 0) {
            if (prop->format == MPV_FORMAT_FLAG) {
                bool d = *(int *)prop->data;
                emit muteChanged(d);
            }
        } else if (strcmp(prop->name, "pause") == 0) {
            if (prop->format == MPV_FORMAT_FLAG) {
                bool d = *(int *)prop->data;
                emit pauseChanged(d);
            }
        }
        break;
    }
    default: ;
        // Ignore uninteresting or unknown events.
    }
}
void MovieNodeOpenGLWorker::onChainSizeChanged(QSize size)
{
    if(size != m_chainSize) {
        m_chainSize = size;
        updateSizes();
    }
}
void MovieNodeOpenGLWorker::updateSizes()
{
    if(!m_videoSize.width() || !m_videoSize.height())
        return;
    if(!m_chainSize.width() || !m_chainSize.height())
        return;
    auto _scale = std::max({
        m_chainSize.width()/qreal(m_videoSize.width())
       ,m_chainSize.height()/qreal(m_videoSize.height())
//       ,qreal(1)
        });
    m_size = m_videoSize * _scale;
}

void MovieNodeOpenGLWorker::drawFrame() {
    if (!m_frames.back()|| m_frames.back()->size() != m_size) {
        auto fmt = QOpenGLFramebufferObjectFormat{};
        fmt.setInternalTextureFormat(GL_RGBA);
        m_frames.back() = QSharedPointer<QOpenGLFramebufferObject>::create(m_size,fmt);
    }
    auto fbo = m_frames.back();
    mpv_opengl_cb_draw(m_mpv_gl, fbo->handle(), fbo->width(), -fbo->height());
    glFlush();
    {
        QMutexLocker locker(&m_rwLock);
        std::rotate(m_frames.begin(),m_frames.end() - 1,m_frames.end());
    }
}

void MovieNodeOpenGLWorker::onVideoChanged() {
    qDebug() << "LOAD" << m_p->m_videoPath;
    if (!m_mpv_gl) return; // Wait for initialization
    QString filename;
    {
        QMutexLocker locker(&m_p->m_stateLock);
        if (m_p->m_videoPath.isEmpty()) return;
        //filename = QString("../resources/videos/%1").arg(m_p->m_videoPath);
        if (m_p->m_videoPath.contains("|")) {
            auto parts = m_p->m_videoPath.split("|");
            filename = QString("%1").arg(parts.at(0));
            for (int i = 1; i + 1 < parts.count(); i += 2) {
                auto k = parts.at(i);
                auto v = parts.at(i + 1);
                // This is a bit of hack, if the videoPath is changed the properties will not be cleared
                mpv_set_property_string(m_mpv, k.toLatin1().data(), v.toLatin1().data());
            }
        } else {
            filename = QString("%1").arg(m_p->m_videoPath);
        }
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

void MovieNodeOpenGLWorker::setPosition(qreal position) {
    command(QVariantList() << "seek" << position << "absolute");
}

void MovieNodeOpenGLWorker::setMute(bool mute) {
    command(QVariantList() << "set" << "mute" << (mute ? "yes" : "no"));
}

void MovieNodeOpenGLWorker::setPause(bool pause) {
    command(QVariantList() << "set" << "pause" << (pause ? "yes" : "no"));
}

MovieNodeOpenGLWorker::~MovieNodeOpenGLWorker() {
    if (m_mpv_gl)
        mpv_opengl_cb_set_update_callback(m_mpv_gl, nullptr, nullptr);
    // Until this call is done, we need to make sure the player remains
    // alive. This is done implicitly with the mpv::qt::Handle instance
    // in this class.
    mpv_opengl_cb_uninit_gl(m_mpv_gl);
}

bool MovieNodeOpenGLWorker::loadBlitShader() {
    auto vertexString = QString{
        "#version 150\n"
        "out vec2 uv;\n"
        "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));\n"
        "void main() {\n"
        "    vec2 vertex = varray[gl_VertexID];\n"
        "    gl_Position = vec4(vertex,0.,1.);\n"
        "    uv = 0.5*(vertex+1.);\n"
        "}\n"};
    auto fragmentString = QString{
        "#version 150\n"
        "uniform sampler2D iVideoFrame;\n"
        "uniform vec2 iResolution;\n"
        "uniform vec2 iVideoResolution;\n"
        "in vec2 uv;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec2 factorFit = iVideoResolution.yx * iResolution.xy / iVideoResolution.xy / iResolution.yx;\n"
        "    vec2 factor = min(factorFit, 1.);\n"
        "    vec2 texUV = (uv - 0.5) * factor + 0.5;\n"
        "    vec2 clamp = (step(0., texUV) - step(1., texUV));\n"
        "    fragColor = texture(iVideoFrame, texUV) * clamp.x * clamp.y;\n"
        "}\n"};

    m_state = QSharedPointer<MovieNodeRenderState>::create();
    auto bs = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

    if (!bs->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
        emit fatal("Could not compile vertex shader");
        return false;
    }
    if (!bs->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
        emit fatal("Could not compile fragment shader");
        return false;
    }
    if (!bs->link()) {
        emit fatal("Could not link shader program");
        return false;
    }
    m_state->m_pass.m_shader = bs;
    m_state->m_ready.store(true);
    return true;
}
void MovieNodeOpenGLWorker::onPrepareState(QSharedPointer<MovieNodeRenderState> state) {
    if(!state || !m_state || state->m_ready.load())
        return;
    state->m_pass.m_shader = copyProgram(m_state->m_pass.m_shader);
    state->m_ready.exchange(true);
}

QString MovieNode::typeName() {
    return "MovieNode";
}

VideoNode *MovieNode::deserialize(Context *context, QJsonObject obj) {
    QString name = obj.value("videoPath").toString();
    if (obj.isEmpty()) {
        return nullptr;
    }
    MovieNode *e = new MovieNode(context, name);
    return e;
}

bool MovieNode::canCreateFromFile(QString filename) {
    return filename.endsWith(".mp4", Qt::CaseInsensitive);
}

VideoNode *MovieNode::fromFile(Context *context, QString filename) {
    MovieNode *e = new MovieNode(context, filename);
    return e;
}

