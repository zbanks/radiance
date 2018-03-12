#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include "OpenGLUtils.h"
#include <QOpenGLFramebufferObject>
#include <QMutex>
#include <QReadWriteLock>
#include <QOpenGLShaderProgram>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>
#include <vector>
#include <array>

class MovieNodeOpenGLWorker;
class MovieNodePrivate;

class MovieNodeRenderState {
public:
    MovieNodeRenderState(QSharedPointer<QOpenGLShaderProgram> shader);

    QSharedPointer<QOpenGLShaderProgram> m_shader;
    QSharedPointer<QOpenGLFramebufferObject> m_output;
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode to provide a video using libMPV
class MovieNode
    : public VideoNode {

    friend class WeakMovieNode;
    friend class MovieNodeOpenGLWorker;

    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged WRITE setPosition)
    Q_PROPERTY(bool mute READ mute NOTIFY muteChanged WRITE setMute)
    Q_PROPERTY(bool pause READ pause NOTIFY pauseChanged WRITE setPause)
    Q_PROPERTY(enum Factor factor READ factor WRITE setFactor NOTIFY factorChanged)

public:
    enum Factor {
        Crop,
        Shrink,
        Zoom
    };
    Q_ENUM(Factor)

    static const float zoomFactor;

    MovieNode(Context *context, QString file, QString name=QString(""));
    MovieNode(const MovieNode &other);
    MovieNode *clone() const override;

    QJsonObject serialize() override;

    GLuint paint(Chain chain, QVector<GLuint> inputTextures) override;

    // These static methods are required for VideoNode creation
    // through the registry

    // A string representation of this VideoNode type
    static QString typeName();

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    static VideoNode *deserialize(Context *context, QJsonObject obj);

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    static bool canCreateFromFile(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    static VideoNode *fromFile(Context *context, QString filename);

    // Returns QML filenames that can be loaded
    // to instantiate custom instances of this VideoNode
    static QMap<QString, QString> customInstantiators();

public slots:
    QSize videoSize();
    QString file();
    QString name();
    qreal position();
    qreal duration();
    bool mute();
    bool pause();
    enum Factor factor();
    void setFile(QString file);
    void setPosition(qreal position);
    void setName(QString name);
    void setMute(bool mute);
    void setPause(bool pause);
    void setFactor(enum Factor factor);

protected slots:
    void onVideoSizeChanged(QSize size);
    void onPositionChanged(qreal position);
    void onDurationChanged(qreal duration);
    void onMuteChanged(bool mute);
    void onPauseChanged(bool pause);

signals:
    void fileChanged(QString file);
    void nameChanged(QString name);
    void videoSizeChanged(QSize size);
    void positionChanged(qreal position);
    void durationChanged(qreal duration);
    void muteChanged(bool mute);
    void pauseChanged(bool pause);
    void factorChanged(enum Factor factor);

protected:
    void chainsEdited(QList<Chain> added, QList<Chain> removed) override;
    void reload();

private:
    QSharedPointer<MovieNodePrivate> d() const;
    MovieNode(QSharedPointer<MovieNodePrivate> other_ptr);
};

class MovieNodePrivate : public VideoNodePrivate {
public:
    MovieNodePrivate(Context *context);

    QString m_file;
    QString m_name;
    QSharedPointer<MovieNodeOpenGLWorker> m_openGLWorker;
    OpenGLWorkerContext *m_openGLWorkerContext;
    QMap<Chain, QSharedPointer<MovieNodeRenderState>> m_renderStates;
    QSharedPointer<QOpenGLShaderProgram> m_blitShader;
    QSize m_videoSize;

    // Max size of any chain, so we know what res to render the video at
    QSize m_maxSize;

    bool m_ready{};
    qreal m_position{};
    qreal m_duration{};
    bool m_mute{true};
    bool m_pause{};
    enum MovieNode::Factor m_factor{MovieNode::Crop};
};

///////////////////////////////////////////////////////////////////////////////

class WeakMovieNode {
public:
    WeakMovieNode();
    WeakMovieNode(const MovieNode &other);
    QSharedPointer<MovieNodePrivate> toStrongRef();

protected:
    QWeakPointer<MovieNodePrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////

class MovieNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    MovieNodeOpenGLWorker(MovieNode p);
    ~MovieNodeOpenGLWorker() override;
    QVector<QSharedPointer<QOpenGLFramebufferObject>> m_frames{BUFFER_COUNT};
    QMutex m_rwLock;

signals:
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
    void positionChanged(qreal position);
    void durationChanged(qreal duration);
    void videoSizeChanged(QSize size);
    void muteChanged(bool mute);
    void pauseChanged(bool pause);

public slots:
    void command(const QVariant &params);
    void drawFrame();
    void setPosition(qreal position);
    void setMute(bool mute);
    void setPause(bool pause);

    // Call this after changing
    // "file"
    void initialize(QString filename);

    // Call this to prepare and add a new renderState
    // if chains change or one is somehow missing
    void addNewState(Chain chain);

protected:
    void handleEvent(mpv_event *event);

    static constexpr int BUFFER_COUNT = 3; // TODO double buffering doesn't quite work for some reason
    WeakMovieNode m_p;
    mpv::qt::Handle m_mpv;
    mpv_opengl_cb_context *m_mpv_gl{}; // TODO check this isn't leaked
    QSize m_size;
    QSize m_videoSize;
    QSize m_chainSize;
    QSharedPointer<MovieNodeRenderState> m_state;

protected slots:
    void onEvent();

private:
    QSharedPointer<QOpenGLShaderProgram> loadBlitShader();
};

