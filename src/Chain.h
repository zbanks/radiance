#pragma once

#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QSharedPointer>
#include <QVector>
#include <QMutex>

class Chain : public QObject {
    Q_OBJECT

    Q_PROPERTY(QSize size READ size CONSTANT);
    Q_PROPERTY(int blankTexture READ blankTexture);
    Q_PROPERTY(int noiseTexture READ noiseTexture);
    Q_PROPERTY(qreal realTime READ realTime);
    Q_PROPERTY(qreal beatTime READ beatTime);
public:
    Chain(QSize size);
   ~Chain() override;
    QSize size();
public slots:
    GLuint noiseTexture();
    GLuint blankTexture();
    QOpenGLVertexArrayObject &vao();
    const QOpenGLVertexArrayObject &vao() const;
    qreal realTime() const;
    qreal beatTime() const;
    void setRealTime(qreal time);
    void setBeatTime(qreal time);

protected:
    QOpenGLTexture m_noiseTexture;
    QOpenGLTexture m_blankTexture;
    qreal                     m_realTime{};
    qreal                     m_beatTime{};
    QOpenGLVertexArrayObject  m_vao;
    QSize m_size;
};
