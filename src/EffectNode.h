#pragma once

#include "VideoNode.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>

class EffectNode : public VideoNode {
    Q_OBJECT

public:
    EffectNode(RenderContext *context, QString name);
    ~EffectNode();

public slots:
    void initialize() override;
    qreal intensity();
    void setIntensity(qreal value);
    void paint(int chain, QVector<QSharedPointer<QOpenGLTexture>>) override;

signals:
    void intensityChanged(qreal value);

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
