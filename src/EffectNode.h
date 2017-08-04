#pragma once

#include "VideoNode.h"
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

public slots:
    void initialize() override;
    qreal intensity();
    QString name();
    void setIntensity(qreal value);
    void setName(QString name);
    void paint(int chain, QVector<QSharedPointer<QOpenGLTexture>>) override;

signals:
    void intensityChanged(qreal value);
    void nameChanged(QString name);

private:
    QVector<QVector<QSharedPointer<QOpenGLTexture> > > fbos;
    QVector<int> m_fboIndex;
    QVector<QSharedPointer<QOpenGLShaderProgram> > m_programs;

    bool loadProgram(QString name);

    qreal m_intensity;
    qreal m_intensityIntegral;
    qreal m_realTime;
    qreal m_realTimeLast;
    QString m_name;
};
