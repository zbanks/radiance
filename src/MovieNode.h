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
    QVector<QOpenGLFramebufferObject *> m_fbos;
    QVector<QMutex *> m_fboLocks;
    int lastIndex();

signals:
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
    void initialized();

public slots:
    void onVideoChanged();
    void command(const QVariant &params);
    void drawFrame();

protected:
    const int m_bufferCount = 2;
    MovieNode *m_p;
    mpv::qt::Handle m_mpv;
    mpv_opengl_cb_context *m_mpv_gl;
    QAtomicInt m_fboIndex;

protected slots:
    void initialize();
    void onDestroyed();

private:
    bool loadBlitShader();
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

    QString m_videoPath;
    QSharedPointer<MovieNodeOpenGLWorker> m_openGLWorker;
    QMap<QSharedPointer<Chain>, QSharedPointer<QOpenGLFramebufferObject>> m_renderFbos;
    QSharedPointer<QOpenGLShaderProgram> m_blitShader;

    bool m_ready;
};
