#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <QOpenGLFramebufferObject>

class EffectNode;

// This struct extends the VideoNodeRenderState
// to add additional state to each render pipeline.
// It adds some intermediate framebuffers
// and an index into them.
struct EffectNodeRenderState {
public:
    QVector<QSharedPointer<QOpenGLFramebufferObject>> m_intermediate;
    int m_textureIndex;
};

///////////////////////////////////////////////////////////////////////////////

// This class extends OpenGLWorker
// to enable shader compilation
// and other initialization
// in a background context
class EffectNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    EffectNodeOpenGLWorker(EffectNode *p);
public slots:   
    // Call this after changing
    // "name"
    void initialize();
signals:
    // This is emitted when it is done
    void initialized();

    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
protected:
    bool loadProgram(QString name);
    EffectNode *m_p;
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode
// to create a video effect
// based on one or more shader programs.
class EffectNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    friend class EffectNodeOpenGLWorker;

public:
    EffectNode();
    EffectNode(const EffectNode &other);
    ~EffectNode();

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

    // Creates a copy of this node
    QSharedPointer<VideoNode> createCopyForRendering() override;

    // Reads back the new render state
    void copyBackRenderState(QSharedPointer<Chain> chain, QSharedPointer<VideoNode> copy) override;

public slots:
    qreal intensity();
    QString name();
    void setIntensity(qreal value);
    void setName(QString name);

protected slots:
    void onInitialized();
    void periodic();
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);

protected:
    QMap<QSharedPointer<Chain>, EffectNodeRenderState> m_renderStates;
    QVector<QSharedPointer<QOpenGLShaderProgram>> m_programs;
    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_realTime;
    qreal m_realTimeLast;
    QString m_name;
    QSharedPointer<EffectNodeOpenGLWorker> m_openGLWorker;
    QTimer m_periodic; // XXX do something better here
    bool m_ready;
};
