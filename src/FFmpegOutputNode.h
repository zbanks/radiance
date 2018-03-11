#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QProcess>
#include "OutputNode.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

class FFmpegOpenGLWorker;
class FFmpegOutputNodePrivate;

class FFmpegOutputNode
    : public OutputNode {
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged);
    Q_PROPERTY(QStringList ffmpegArguments READ ffmpegArguments WRITE setFFmpegArguments NOTIFY ffmpegArgumentsChanged);

    friend class WeakFFmpegOutputNode;
    friend class FFmpegOpenGLWorker;

public:
    FFmpegOutputNode(Context *context, QSize chainSize, qreal fps=30.);
    FFmpegOutputNode(const FFmpegOutputNode &other);
    FFmpegOutputNode *clone() const override;

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
    void start();
    void stop();
    void force();

    bool recording();
    void setRecording(bool recording);
    QStringList ffmpegArguments();
    void setFFmpegArguments(QStringList ffmpegArguments);
    qreal fps();
    void setFps(qreal fps);

signals:
    void initialize();
    void frame(QSize size, QByteArray frame);
    void recordingChanged(bool recording);
    void ffmpegArgumentsChanged(QStringList ffmpegArguments);
    void fpsChanged(qreal fps);

private:
    FFmpegOutputNode(QSharedPointer<FFmpegOutputNodePrivate> other_ptr);
    QSharedPointer<FFmpegOutputNodePrivate> d() const;
};

///////////////////////////////////////////////////////////////////////////////

class WeakFFmpegOutputNode {
public:
    WeakFFmpegOutputNode();
    WeakFFmpegOutputNode(const FFmpegOutputNode &other);
    QSharedPointer<FFmpegOutputNodePrivate> toStrongRef();

protected:
    QWeakPointer<FFmpegOutputNodePrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////

class FFmpegOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    FFmpegOpenGLWorker(FFmpegOutputNode p);

public slots:
    void initialize(QSize size);
    void setFFmpegArguments(QStringList ffmpegArguments);
    void setFps(qreal fps);
    void start();
    void stop();
    void renderFrame();

signals:
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
    void initialized();
    void frame(QSize size, QByteArray pixelBuffer);

protected:
    QSharedPointer<QOpenGLShaderProgram> loadBlitShader();

private:
    WeakFFmpegOutputNode m_p;
    bool m_initialized;
    QSharedPointer<QTimer> m_timer;
    bool m_useTimer;
    QByteArray m_pixelBuffer;
    QSize m_size;
    QSharedPointer<QOpenGLShaderProgram> m_shader;
    QSharedPointer<QOpenGLFramebufferObject> m_fbo;
    QSharedPointer<QProcess> m_ffmpeg;
};

class FFmpegOutputNodePrivate : public OutputNodePrivate {
public:
    FFmpegOutputNodePrivate(Context *context, QSize chainSize);

    OpenGLWorkerContext *m_workerContext{};
    QSharedPointer<FFmpegOpenGLWorker> m_worker;

    bool m_recording;
    QStringList m_ffmpegArguments;
    qreal m_fps;
};
