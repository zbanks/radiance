#pragma once

#include "VideoNode.h"
#include "FramebufferObject.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>

class EffectNode : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    EffectNode();
    ~EffectNode();

    static constexpr qreal MAX_INTEGRAL = 1024;
    static constexpr qreal FPS = 60;

public slots:
    void initialize() override;
    qreal intensity();
    QString name();
    void setIntensity(qreal value);
    void setName(QString name);
    void paint(int chain, QVector<QSharedPointer<QOpenGLTexture>> inputTextures) override;

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);

private:
    // First index: chain
    // Second index: layer
    QVector<QVector<QSharedPointer<QOpenGLTexture> > > m_intermediate;
    // Indexed by chain
    QVector<int> m_textureIndex;
    QVector<QSharedPointer<QOpenGLShaderProgram> > m_programs;
    QVector<QSharedPointer<FramebufferObject> > m_fbos;

    bool loadProgram(QString name);

    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_realTime;
    qreal m_realTimeLast;
    QString m_name;
    bool m_initialized;
};
