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
#include "Context.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

// This zoom factor eliminates top / bottom black bars
// on videos that are recorded in 21:9 aspect
// and letterboxed to 16:9
const float MovieNode::zoomFactor = 16. / 21.;

MovieNode::MovieNode(Context *context, QString file, QString name)
    : VideoNode(context)
{
    attachSignals();
    m_openGLWorkerContext = new OpenGLWorkerContext(context->threaded());
    m_openGLWorker = QSharedPointer<MovieNodeOpenGLWorker>(new MovieNodeOpenGLWorker(qSharedPointerCast<MovieNode>(sharedFromThis())), &QObject::deleteLater);
    connect(m_openGLWorker.data(), &QObject::destroyed, m_openGLWorkerContext, &QObject::deleteLater);

    connect(m_openGLWorker.data(), &MovieNodeOpenGLWorker::videoSizeChanged, this, &MovieNode::onVideoSizeChanged);
    connect(m_openGLWorker.data(), &MovieNodeOpenGLWorker::positionChanged, this, &MovieNode::onPositionChanged);
    connect(m_openGLWorker.data(), &MovieNodeOpenGLWorker::durationChanged, this, &MovieNode::onDurationChanged);
    connect(m_openGLWorker.data(), &MovieNodeOpenGLWorker::muteChanged, this, &MovieNode::onMuteChanged);
    connect(m_openGLWorker.data(), &MovieNodeOpenGLWorker::pauseChanged, this, &MovieNode::onPauseChanged);

    setInputCount(1);
    setName(name);
    setFile(file);
}

QJsonObject MovieNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("file", file());
    o.insert("name", name());
    return o;
}

void MovieNode::chainsEdited(QList<ChainSP> added, QList<ChainSP> removed) {
    for (auto chain : added) {
        auto result = QMetaObject::invokeMethod(m_openGLWorker.data(), "addNewState", Q_ARG(ChainSP, chain));
        Q_ASSERT(result);
    }
    {
        QMutexLocker locker(&m_stateLock);
        for (auto chain : removed) {
            m_renderStates.remove(chain);
        }
        auto size = QSize{};
        for(auto chain : m_chains) {
            auto csize = chain->size();
            size.setWidth(std::max(csize.width(), size.width()));
            size.setHeight(std::max(csize.height(), size.height()));
        }
        m_maxSize = size;
    }
}

QString MovieNode::file() {
    QMutexLocker locker(&m_stateLock);
    return m_file;
}

QString MovieNode::name() {
    QMutexLocker locker(&m_stateLock);
    return m_name;
}

qreal MovieNode::position() {
    QMutexLocker locker(&m_stateLock);
    return m_position;
}

qreal MovieNode::duration() {
    QMutexLocker locker(&m_stateLock);
    return m_duration;
}

QSize MovieNode::videoSize() {
    QMutexLocker locker(&m_stateLock);
    return m_videoSize;
}

bool MovieNode::mute() {
    QMutexLocker locker(&m_stateLock);
    return m_mute;
}

bool MovieNode::pause() {
    QMutexLocker locker(&m_stateLock);
    return m_pause;
}

enum MovieNode::Factor MovieNode::factor() {
    QMutexLocker locker(&m_stateLock);
    return m_factor;
}

void MovieNode::onVideoSizeChanged(QSize size) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if(size != m_videoSize) {
            m_videoSize = size;
            changed = true;
        }
    }
    if (changed) emit videoSizeChanged(size);
}

void MovieNode::onPositionChanged(qreal position) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if(position != m_position) {
            m_position = position;
            changed = true;
        }
    }
    if (changed) emit positionChanged(position);
}

void MovieNode::onDurationChanged(qreal duration) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if(duration != m_duration) {
            m_duration = duration;
            changed = true;
        }
    }
    if (changed) emit durationChanged(duration);
}

void MovieNode::onMuteChanged(bool mute) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if(mute != m_mute) {
            m_mute = mute;
            changed = true;
        }
    }
    if (changed) emit muteChanged(mute);
}

void MovieNode::onPauseChanged(bool pause) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if(pause != m_pause) {
            m_pause = pause;
            changed = true;
        }
    }
    if (changed) emit pauseChanged(pause);
}

void MovieNode::reload() {
    QString filename;
    setNodeState(VideoNode::Loading);

    {
        QMutexLocker locker(&m_stateLock);
        m_ready = false;
        filename = m_file;
    }

    bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize", Q_ARG(QString, filename));
    Q_ASSERT(result);
}

void MovieNode::setFile(QString file) {
    if (!file.contains("://")) {
        file = Paths::contractLibraryPath(file);
    }
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if(file != m_file) {
            m_file = file;
            changed = true;
        }
    }
    if (changed) {
        reload();
        emit fileChanged(file);
    }
}

void MovieNode::setName(QString name) {
    bool changed = false;
    {
        QMutexLocker locker(&m_stateLock);
        if(name != m_name) {
            m_name = name;
            changed = true;
        }
    }
    if (changed) emit nameChanged(name);
}

void MovieNode::setPosition(qreal position) {
    QMetaObject::invokeMethod(m_openGLWorker.data(), "setPosition", Qt::QueuedConnection, Q_ARG(qreal, position));
}

void MovieNode::setMute(bool mute) {
    QMetaObject::invokeMethod(m_openGLWorker.data(), "setMute", Qt::QueuedConnection, Q_ARG(bool, mute));
}

void MovieNode::setPause(bool pause) {
    QMetaObject::invokeMethod(m_openGLWorker.data(), "setPause", Qt::QueuedConnection, Q_ARG(bool, pause));
}

void MovieNode::setFactor(enum Factor factor) {
    {
        QMutexLocker locker(&m_stateLock);
        if (m_factor == factor) return;
        m_factor = factor;
    }
    emit factorChanged(factor);
}

static void *get_proc_address(void *ctx, const char *name) {
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return NULL;
    return (void *)glctx->getProcAddress(QByteArray(name));
}

GLuint MovieNode::paint(ChainSP chain, QVector<GLuint> inputTextures) {
    GLuint outTexture = inputTextures.at(0);
    enum Factor f;

    QSharedPointer<MovieNodeRenderState> renderState;
    {
        QMutexLocker locker(&m_stateLock);
        if (!m_ready) {
            //qDebug() << this << "is not ready";
            return outTexture;
        }
        //qDebug() << "Looking up" << chain << "in" << m_renderStates << "of" << this;
        if (!m_renderStates.contains(chain)) { // This check doesn't seem to work
            qDebug() << this << "does not have chain" << chain;
            return outTexture;
        }
        renderState = m_renderStates[chain];
        f = m_factor;
    }

    auto renderFbo = renderState->m_output;

    // FBO creation must happen here, and not in initialize,
    // because FBOs are not shared among contexts.
    // Textures are, however, so in the future maybe we can move
    // texture creation to initialize()
    // and leave lightweight FBO creation here
    if(!renderFbo || renderFbo->size() != chain->size()) {
        auto fmt = QOpenGLFramebufferObjectFormat{};
        fmt.setInternalTextureFormat(GL_RGBA);
        renderFbo = renderState->m_output = QSharedPointer<QOpenGLFramebufferObject>::create(chain->size(), fmt);
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

            auto blitShader = renderState->m_shader;
            blitShader->bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fboi->texture());
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

            blitShader->setUniformValue("iVideoFrame", 0);

            auto factorFitX = (float)fboi->height() * renderFbo->width() / fboi->width() / renderFbo->height();
            auto factorFitY = (float)fboi->width() * renderFbo->height() / fboi->height() / renderFbo->width();

            switch(f) {
                case Shrink:
                    factorFitX = qMax(factorFitX, 1.f);
                    factorFitY = qMax(factorFitY, 1.f);
                    break;
                case Zoom:
                    factorFitX = factorFitX * zoomFactor;
                    factorFitY = zoomFactor;
                    break;
                case Crop:
                default:
                    factorFitX = qMin(factorFitX, 1.f);
                    factorFitY = qMin(factorFitY, 1.f);
                    break;
            }
            blitShader->setUniformValue("iFactor", GLfloat(factorFitX), GLfloat(factorFitY));

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            renderFbo->release();
            outTexture = renderFbo->texture();

            blitShader->release();
        }
    }

    return outTexture;
}

// MovieNodeOpenGLWorker methods

MovieNodeOpenGLWorker::MovieNodeOpenGLWorker(QSharedPointer<MovieNode> p)
    : OpenGLWorker(p->m_openGLWorkerContext)
    , m_p(p)
{
    connect(this, &MovieNodeOpenGLWorker::message, p.data(), &MovieNode::message);
    connect(this, &MovieNodeOpenGLWorker::warning, p.data(), &MovieNode::warning);
    connect(this, &MovieNodeOpenGLWorker::error,   p.data(), &MovieNode::error);
}

static void requestUpdate(void *ctx) {
    QMetaObject::invokeMethod((MovieNodeOpenGLWorker*)ctx, "drawFrame", Qt::QueuedConnection);
}

static void requestWakeup(void *ctx) {
    QMetaObject::invokeMethod((MovieNodeOpenGLWorker*)ctx, "onEvent", Qt::QueuedConnection);
}

void MovieNodeOpenGLWorker::initialize(QString filename) {
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // MovieNode was deleted

    // I am not sure if this method is okay with being called multiple times.
    // It may need some changes to un-load the previous MPV context
    // and load up the new one
    // in order to support changing the "file" property
    // or re-loading

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

    auto shader = loadBlitShader(p);
    if (shader.isNull()) return;

    qDebug() << "LOAD" << filename;

    if (filename.isEmpty()) return;
    if (!filename.contains("://")) {
        filename = Paths::expandLibraryPath(filename);
    }
    if (filename.contains("|")) {
        auto parts = filename.split("|");
        filename = parts.at(0);
        for (int i = 1; i + 1 < parts.count(); i += 2) {
            auto k = parts.at(i);
            auto v = parts.at(i + 1);
            mpv_set_property_string(m_mpv, k.toLatin1().data(), v.toLatin1().data());
        }
    }

    QFileInfo check_file(filename);
    if(!filename.contains("://") && !(check_file.exists() && check_file.isFile())) {
        qWarning() << "Could not find" << filename;
        emit warning(QString("Could not find %1").arg(filename));
    }

    command(QStringList() << "loadfile" << filename);

    setMute(p->mute());
    setPause(p->pause());

    qDebug() << "Successfully loaded video" << filename;

    // We prepare the state for all chains that exist upon creation
    auto chains = p->chains();
    QMap<ChainSP, QSharedPointer<MovieNodeRenderState>> states;

    for (auto chain : chains) {
        states.insert(chain, QSharedPointer<MovieNodeRenderState>::create(shader));
    }

    // Swap out the newly loaded stuff
    {
        QMutexLocker locker(&p->m_stateLock);
        p->m_renderStates.clear();

        // Chains may have been deleted while we were preparing the states
        auto realChains = p->m_chains;
        for (auto realChain: realChains) {
            if (states.contains(realChain)) {
                p->m_renderStates.insert(realChain, states.value(realChain));
            }
        }

        p->m_blitShader = shader;
    }
}

// Invoke this method when a ChainSP gets added
// (or when the state for a given chain is somehow missing)
// It will create the new state asynchronously and add it when it is ready.
void MovieNodeOpenGLWorker::addNewState(ChainSP c) {
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // MovieNode was deleted

    QSharedPointer<QOpenGLShaderProgram> shader;
    {
        QMutexLocker locker(&p->m_stateLock);
        // Don't make states that we don't have to
        if (!p->m_ready) return;
        if (!p->m_chains.contains(c)) return;
        if (p->m_renderStates.contains(c)) return;
        shader = p->m_blitShader;
    }

    makeCurrent();
    auto state = QSharedPointer<MovieNodeRenderState>::create(shader);

    {
        QMutexLocker locker(&p->m_stateLock);
        // Check that everything is still OK
        if (!p->m_ready) return;
        if (!p->m_chains.contains(c)) return;
        if (p->m_renderStates.contains(c)) return;
        p->m_renderStates.insert(c, state);
    }
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
                auto p = m_p.toStrongRef();
                if (p.isNull()) return; // MovieNode was deleted
                p->setNodeState(VideoNode::Ready);
                {
                    QMutexLocker locker(&p->m_stateLock);
                    p->m_ready = true;
                }
            }
        } else if (strcmp(prop->name, "video-params/w") == 0) {
            if (prop->format == MPV_FORMAT_INT64) {
                qint64 d = *(qint64 *)prop->data;
                m_videoSize.setWidth(d);
                emit videoSizeChanged(m_videoSize);
            }
        } else if (strcmp(prop->name, "video-params/h") == 0) {
            if (prop->format == MPV_FORMAT_INT64) {
                qint64 d = *(qint64 *)prop->data;
                m_videoSize.setHeight(d);
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

void MovieNodeOpenGLWorker::drawFrame() {
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // MovieNode was deleted

    {
        QMutexLocker locker(&p->m_stateLock);
        if(!m_videoSize.width() || !m_videoSize.height())
            return;
        if(!m_chainSize.width() || !m_chainSize.height())
            return;
        auto scale = std::max({
            p->m_maxSize.width() / qreal(m_videoSize.width()),
            p->m_maxSize.height() / qreal(m_videoSize.height()),
            //qreal(1)
            });
        m_size = m_videoSize * scale;
    }

    if (!m_frames.back() || m_frames.back()->size() != m_size) {
        auto fmt = QOpenGLFramebufferObjectFormat{};
        fmt.setInternalTextureFormat(GL_RGBA);
        m_frames.back() = QSharedPointer<QOpenGLFramebufferObject>::create(m_size, fmt);
    }
    auto fbo = m_frames.back();
    mpv_opengl_cb_draw(m_mpv_gl, fbo->handle(), fbo->width(), -fbo->height());
    glFlush();
    {
        QMutexLocker locker(&m_rwLock);
        std::rotate(m_frames.begin(), m_frames.end() - 1, m_frames.end());
    }
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

QSharedPointer<QOpenGLShaderProgram> MovieNodeOpenGLWorker::loadBlitShader(MovieNodeSP p) {
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
        "uniform vec2 iFactor;\n"
        "in vec2 uv;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec2 texUV = (uv - 0.5) * iFactor + 0.5;\n"
        "    vec2 clamp = (step(0., texUV) - step(1., texUV));\n"
        "    fragColor = texture(iVideoFrame, texUV) * clamp->x * clamp->y;\n"
        "}\n"};

    auto shader = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

    if (!shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
        emit error("Could not compile vertex shader");
        p->setNodeState(VideoNode::Broken);
        return nullptr;
    }
    if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
        emit error("Could not compile fragment shader");
        p->setNodeState(VideoNode::Broken);
        return nullptr;
    }
    if (!shader->link()) {
        emit error("Could not link shader program");
        p->setNodeState(VideoNode::Broken);
        return nullptr;
    }

    return shader;
}

QString MovieNode::typeName() {
    return "MovieNode";
}

VideoNodeSP *MovieNode::deserialize(Context *context, QJsonObject obj) {
    QString file = obj.value("file").toString();
    if (obj.isEmpty()) {
        return nullptr;
    }
    auto e = new MovieNode(context, file);
    auto name = obj.value("name").toString();
    if (!name.isEmpty()) {
        e->setName(name);
    }
    return new VideoNodeSP(e);
}

bool MovieNode::canCreateFromFile(QString filename) {
    QStringList extensions({".mp4", ".mkv"});
    for (auto extension = extensions.begin(); extension != extensions.end(); extension++) {
        if (filename.endsWith(*extension, Qt::CaseInsensitive)) return true;
    }
    return false;
}

VideoNodeSP *MovieNode::fromFile(Context *context, QString filename) {
    auto e = new MovieNode(context, filename);
    if (e != nullptr) {
        e->setName(QFileInfo(e->file()).baseName());
    }
    return new VideoNodeSP(e);
}

QMap<QString, QString> MovieNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("Youtube", "YoutubeInstantiator.qml");
    m.insert("MPV", "MPVInstantiator.qml");
    return m;
}

MovieNodeRenderState::MovieNodeRenderState(QSharedPointer<QOpenGLShaderProgram> shader)
    : m_shader(shader)
{
}
