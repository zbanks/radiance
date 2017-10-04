#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLFramebufferObject>
#include <QMutex>
#include <QTimer>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>

class MovieNode;

///////////////////////////////////////////////////////////////////////////////

// This class extends OpenGLWorker
// to enable shader compilation
// and other initialization
// in a background context
class MovieNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    MovieNodeOpenGLWorker(MovieNode *p);

public slots:
    void initialize();

signals:
    // This is emitted when it is done
    void initialized();

    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
protected:
    void loadVideo();
    MovieNode *m_p;
protected slots:
    void onDestroyed();
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode to provide a video using libMPV
class MovieNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(QString videoPath READ videoPath WRITE setVideoPath NOTIFY videoPathChanged)

    friend class MovieNodeOpenGLWorker;

public:
    MovieNode();
    MovieNode(const MovieNode &other);
    ~MovieNode();

    QSharedPointer<VideoNode> createCopyForRendering() override;
    void copyBackRenderState(QSharedPointer<Chain> chain, QSharedPointer<VideoNode> copy) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

public slots:
    QString videoPath();
    void setVideoPath(QString videoPath);

protected slots:
    void onInitialized();

signals:
    void videoPathChanged(QString videoPath);

protected:
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;
    void command(const QVariant &params);

    QString m_videoPath;
    QSharedPointer<MovieNodeOpenGLWorker> m_openGLWorker;
    QMap<QSharedPointer<Chain>, QSharedPointer<QOpenGLFramebufferObject>> m_renderFbos;
    mpv::qt::Handle m_mpv;
    mpv_opengl_cb_context *m_mpv_gl;

    bool m_ready;
};
