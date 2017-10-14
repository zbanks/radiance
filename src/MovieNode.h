#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLFramebufferObject>
#include <QMutex>
#include <QOpenGLShaderProgram>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>

class MovieNode;

///////////////////////////////////////////////////////////////////////////////

class MovieNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    MovieNodeOpenGLWorker(MovieNode *p);
    ~MovieNodeOpenGLWorker() override;
    QVector<QOpenGLFramebufferObject *> m_fbos;
    QVector<QMutex *> m_fboLocks;
    int lastIndex();

signals:
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
    void positionChanged(qreal position);
    void durationChanged(qreal duration);
    void videoSizeChanged(QSize size);
    void muteChanged(bool mute);
    void initialized();

public slots:
    void onVideoChanged();
    void command(const QVariant &params);
    void drawFrame();
    void setPosition(qreal position);
    void setMute(bool mute);

protected:
    void handleEvent(mpv_event *event);

    static constexpr int BUFFER_COUNT = 3; // TODO double buffering doesn't quite work for some reason
    MovieNode *m_p;
    mpv::qt::Handle m_mpv;
    mpv_opengl_cb_context *m_mpv_gl;
    QSize m_size;
    QAtomicInt m_fboIndex;

protected slots:
    void initialize();
    void onEvent();

private:
    bool loadBlitShader();
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode to provide a video using libMPV
class MovieNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(QString videoPath READ videoPath WRITE setVideoPath NOTIFY videoPathChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)
    Q_PROPERTY(qreal duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged WRITE setPosition)
    Q_PROPERTY(bool mute READ mute NOTIFY muteChanged WRITE setMute)

    friend class MovieNodeOpenGLWorker;

public:
    MovieNode();
    MovieNode(const MovieNode &other);
    ~MovieNode();

    QSharedPointer<VideoNode> createCopyForRendering() override;
    void copyBackRenderState(QSharedPointer<Chain> chain, QSharedPointer<VideoNode> copy) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

public slots:
    QSize videoSize();
    QString videoPath();
    qreal position();
    qreal duration();
    bool mute();
    void setVideoPath(QString videoPath);
    void setPosition(qreal position);
    void setMute(bool mute);

protected slots:
    void onInitialized();
    void onVideoSizeChanged(QSize size);
    void onPositionChanged(qreal position);
    void onDurationChanged(qreal duration);
    void onMuteChanged(bool mute);

signals:
    void videoPathChanged(QString videoPath);
    void videoSizeChanged(QSize size);
    void positionChanged(qreal position);
    void durationChanged(qreal duration);
    void muteChanged(bool mute);

protected:
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;

    QString m_videoPath;
    MovieNodeOpenGLWorker *m_openGLWorker;
    QMap<QSharedPointer<Chain>, QSharedPointer<QOpenGLFramebufferObject>> m_renderFbos;
    QSharedPointer<QOpenGLShaderProgram> m_blitShader;
    OpenGLWorkerContext *m_openGLWorkerContext;
    QSize m_videoSize;

    bool m_ready;
    qreal m_position;
    qreal m_duration;
    bool m_mute;
};
