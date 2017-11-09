#pragma once

#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QSharedPointer>
#include <QVector>
#include <QMutex>

class Chain;

class ChainOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    ChainOpenGLWorker(Chain* p);
public slots:
    void initialize(QSize);
signals:
    void initialized(int, int);
protected:
    void createBlankTexture(QSize);
    void createNoiseTexture(QSize);
    QOpenGLTexture m_noiseTexture{QOpenGLTexture::Target2D};
    QOpenGLTexture m_blankTexture{QOpenGLTexture::Target2D};
};

///////////////////////////////////////////////////////////////////////////////

class Chain : public QObject {
    Q_OBJECT

    friend class ChainOpenGLWorker;

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
    GLuint noiseTexture() const;
    GLuint blankTexture() const;
    QOpenGLVertexArrayObject &vao();
    const QOpenGLVertexArrayObject &vao() const;
    qreal realTime() const;
    qreal beatTime() const;
    void setRealTime(qreal time);
    void setBeatTime(qreal time);
protected slots:
    void onInitialized(int, int);

protected:
    bool m_initialized{false};
    GLuint m_noiseTextureId{};
    GLuint m_blankTextureId{};
    qreal                     m_realTime{};
    qreal                     m_beatTime{};
    QOpenGLVertexArrayObject  m_vao;
    ChainOpenGLWorker        *m_openGLWorker{};
    QSize m_size;
};
