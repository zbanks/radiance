#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include "OpenGLUtils.h"
#include "Context.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <QOpenGLFramebufferObject>

class EffectNodeOpenGLWorker;
class EffectNodePrivate;

class EffectNodeRenderState {
public:
    EffectNodeRenderState(QVector<QSharedPointer<QOpenGLShaderProgram>> shaders);

    struct Pass {
        QSharedPointer<QOpenGLFramebufferObject> m_output;
        QSharedPointer<QOpenGLShaderProgram> m_shader;
    };

    QVector<Pass> m_passes;

    QSharedPointer<QOpenGLFramebufferObject> m_extra;
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode
// to create a video effect
// based on one or more shader programs.
class EffectNode
    : public VideoNode {

    friend class WeakEffectNode;
    friend class EffectNodeOpenGLWorker;

    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(double frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)

public:
    EffectNode(Context *context, QString name);
    EffectNode(const EffectNode &other);
    EffectNode *clone() const override;

    QJsonObject serialize() override;

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

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
    qreal intensity();
    QString name();
    QString file();
    double frequency();
    void setIntensity(qreal value);
    void setFile(QString file);
    void setFrequency(double frequency);
    void reload();

protected slots:
    void onPeriodic();
    void chainsEdited(QList<Chain> added, QList<Chain> removed) override;

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);
    void fileChanged(QString file);
    void frequencyChanged(double frequency);

private:
    QSharedPointer<EffectNodePrivate> d() const;
    EffectNode(QSharedPointer<EffectNodePrivate> other_ptr);
};

class EffectNodePrivate : public VideoNodePrivate {
public:
    EffectNodePrivate(Context *context);

    QMap<Chain, QSharedPointer<EffectNodeRenderState>> m_renderStates;
    qreal m_intensity{};
    qreal m_intensityIntegral{};
    qreal m_beatLast{};
    qreal m_realTime{};
    qreal m_realTimeLast{};
    QString m_file;
    QSharedPointer<EffectNodeOpenGLWorker> m_openGLWorker; // Not shared
    QTimer m_periodic; // XXX do something better here
    bool m_ready{};
    double m_frequency{};
    QVector<QSharedPointer<QOpenGLShaderProgram>> m_shaders;
};

///////////////////////////////////////////////////////////////////////////////

class WeakEffectNode {
public:
    WeakEffectNode();
    WeakEffectNode(const EffectNode &other);
    QSharedPointer<EffectNodePrivate> toStrongRef();

protected:
    QWeakPointer<EffectNodePrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////

// This class extends OpenGLWorker
// to enable shader compilation
// and other initialization
// in a background context
class EffectNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    EffectNodeOpenGLWorker(EffectNode p);

public slots:
    // Call this after changing
    // "file"
    void initialize(QVector<QStringList> passes);

    // Call this to prepare and add a new renderState
    // if chains change or one is somehow missing
    void addNewState(Chain chain);

signals:
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);

protected:
    bool loadProgram(QString file);

private:
    WeakEffectNode m_p;
};
