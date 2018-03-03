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
    // "file"
    void initialize(QVector<QStringList> passes);
    void onPrepareState(QSharedPointer<EffectNodeRenderState> state);
signals:
    // This is emitted when it is done
    void initialized();
    void prepareState(QSharedPointer<EffectNodeRenderState> state);

    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
protected:
    bool loadProgram(QString file);
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
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(double frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)

    friend class EffectNodeOpenGLWorker;

public:
    EffectNode(Context *context, QString name);
    EffectNode(const EffectNode &other);
    ~EffectNode();

    QJsonObject serialize() override;

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

    GLuint paint(Chain chain, QVector<GLuint> inputTextures) override;

    // Creates a copy of this node
    QSharedPointer<VideoNode> createCopyForRendering(Chain chain) override;

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
    void onInitialized();
    void periodic();
    void chainsEdited(QList<Chain> added, QList<Chain> removed) override;

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);
    void fileChanged(QString file);
    void frequencyChanged(double frequency);

protected:
    QMap<Chain, QSharedPointer<EffectNodeRenderState>> m_renderStates;
    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_beatLast;
    qreal m_realTime;
    qreal m_realTimeLast;
    QString m_file;
    QSharedPointer<EffectNodeOpenGLWorker> m_openGLWorker;
    QTimer m_periodic; // XXX do something better here
    bool m_ready;
    double m_frequency;
};
