#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>

class EffectNode;

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
    ~EffectNode();

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

    void paint(int chain, QVector<GLuint> inputTextures) override;

    // Called from OpenGLWorker
    void initialize(QOpenGLFunctions *glFuncs);

public slots:
    qreal intensity();
    QString name();
    void setIntensity(qreal value);
    void setName(QString name);

protected slots:
    void onInitialized();

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);

private:
    // First index: chain
    // Second index: layer
    QVector<QVector<QSharedPointer<QOpenGLFramebufferObject>>> m_intermediate;
    // Indexed by chain
    QVector<int> m_textureIndex;
    QVector<QSharedPointer<QOpenGLShaderProgram>> m_programs;

    bool loadProgram(QString name);

    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_realTime;
    qreal m_realTimeLast;
    QString m_name;
    bool m_initialized;
    EffectNodeOpenGLWorker m_openGLWorker;
};
