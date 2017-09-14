#pragma once

#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QVector>
#include <QMutex>

class Chain;

class ChainOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    ChainOpenGLWorker(Chain* p);
public slots:
    void initialize();
signals:
    void initialized();
protected:
    void createBlankTexture();
    void createNoiseTexture();
    Chain *m_p;
};

///////////////////////////////////////////////////////////////////////////////

class Chain : public QObject {
    Q_OBJECT

    friend class ChainOpenGLWorker;

public:
    Chain(QSize size);
   ~Chain() override;
    QSize size();
    GLuint noiseTexture();
    GLuint blankTexture();

protected slots:
    void onInitialized();

protected:
    bool m_initialized;
    QOpenGLTexture m_noiseTexture;
    QOpenGLTexture m_blankTexture;
    ChainOpenGLWorker m_openGLWorker;
    QSize m_size;
};
