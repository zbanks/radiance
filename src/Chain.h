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
//    Chain *m_p;
};

///////////////////////////////////////////////////////////////////////////////

class Chain : public QObject {
    Q_OBJECT

    friend class ChainOpenGLWorker;

    Q_PROPERTY(QSize size READ size CONSTANT);
    Q_PROPERTY(int blankTexture READ blankTexture);
    Q_PROPERTY(int noiseTexture READ noiseTexture);
public:
    Chain(QSize size);
   ~Chain() override;
    QSize size();
    GLuint noiseTexture();
    GLuint blankTexture();
    QOpenGLVertexArrayObject &vao();
    const QOpenGLVertexArrayObject &vao() const;
protected slots:
    void onInitialized(int, int);

protected:
    bool m_initialized;
    GLuint m_noiseTextureId{};
    GLuint m_blankTextureId{};
    QOpenGLVertexArrayObject  m_vao;
    ChainOpenGLWorker        *m_openGLWorker;
    QSize m_size;
};
