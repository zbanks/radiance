#pragma once

#include "VideoNode.h"
#include "VideoNodeFactory.h"
#include "OpenGLWorker.h"
#include "OpenGLUtils.h"
#include "Context.h"
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
class EffectNodeRenderState {
    Q_GADGET
public:
    using size_type = std::vector<Pass>::size_type;
    using reference = std::vector<Pass>::reference;
    using const_reference = std::vector<Pass>::const_reference;
    using difference_type = std::vector<Pass>::difference_type;

    std::atomic<bool> m_ready{false};
    std::vector<Pass> m_passes;
    QSharedPointer<QOpenGLFramebufferObject> m_extra;

    bool ready() const { return m_ready.load();}
    size_type size() const { return m_passes.size();}
    const_reference at(difference_type x) const { return m_passes.at(x);}
    const_reference operator[](difference_type x) const { return m_passes[x];}
    reference operator[](difference_type x) { return m_passes[x];}
};
Q_DECLARE_METATYPE(QSharedPointer<EffectNodeRenderState>);

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
    void initialize(QString);
    void onPrepareState(QSharedPointer<EffectNodeRenderState> state);
signals:
    // This is emitted when it is done
    void initialized();
    void prepareState(QSharedPointer<EffectNodeRenderState> state);

    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
protected:
    bool loadProgram(QString name);
    QSharedPointer<EffectNodeRenderState> m_state;
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
    EffectNode(Context *context, QString name);
    EffectNode(const EffectNode &other);
    ~EffectNode();

    QJsonObject serialize() override;

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

    // Creates a copy of this node
    QSharedPointer<VideoNode> createCopyForRendering(QSharedPointer<Chain> chain) override;

public slots:
    qreal intensity();
    QString name();
    void setIntensity(qreal value);
    void setName(QString name);
    void reload();

protected slots:
    void onInitialized();
    void periodic();
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);

protected:
    QMap<QSharedPointer<Chain>, QSharedPointer<EffectNodeRenderState>> m_renderStates;
    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_beatLast;
    qreal m_realTime;
    qreal m_realTimeLast;
    QString m_name;
    QSharedPointer<EffectNodeOpenGLWorker> m_openGLWorker;
    QTimer m_periodic; // XXX do something better here
    bool m_ready;
};

//////////////////////////////////////////////////////////////////////////////

class EffectNodeFactory : public VideoNodeFactory {

public:
    QString typeName() override;
    VideoNode *deserialize(Context *context, QJsonObject obj) override;
    bool canCreateFromFile(QString filename) override;
    VideoNode *fromFile(Context *context, QString filename) override;
};
