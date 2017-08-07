#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>

class EffectNode;

struct EffectNodeRenderState : public VideoNodeRenderState {
public:
    QVector<QSharedPointer<QOpenGLFramebufferObject>> m_intermediate;
    int m_textureIndex;
};

class EffectNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    EffectNodeOpenGLWorker(EffectNode *p);
public slots:
    void initialize();
signals:
    void initialized();
protected:
    EffectNode *m_p;
};

class EffectNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    EffectNode();
    EffectNode(const EffectNode &other);
    ~EffectNode();

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

    void paint(int chain, QVector<GLuint> inputTextures) override;

    // Called from OpenGLWorker
    void initialize(QOpenGLFunctions *glFuncs);

    // Get the output texture of this EffectNode
    GLuint texture(int chain);

    // Creates a copy of this node
    QSharedPointer<VideoNode> createCopyForRendering();

    // Reads back the new render state
    void copyBackRenderState(int chain, QSharedPointer<VideoNode> copy);

public slots:
    qreal intensity();
    QString name();
    void setIntensity(qreal value);
    void setName(QString name);

protected slots:
    void onInitialized();
    void periodic();

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);

private:
    QVector<EffectNodeRenderState> m_renderStates;
    QVector<QSharedPointer<QOpenGLShaderProgram>> m_programs;

    bool loadProgram(QString name);

    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_realTime;
    qreal m_realTimeLast;
    QString m_name;
    bool m_initialized;
    QSharedPointer<EffectNodeOpenGLWorker> m_openGLWorker;
    QMutex m_stateLock;
    QTimer m_periodic;
};
