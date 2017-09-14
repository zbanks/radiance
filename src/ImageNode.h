#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>

class ImageNode;

///////////////////////////////////////////////////////////////////////////////

// This class extends OpenGLWorker
// to enable shader compilation
// and other initialization
// in a background context
class ImageNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    ImageNodeOpenGLWorker(ImageNode *p);

public slots:
    void initialize();

signals:
    // This is emitted when it is done
    void initialized();

    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
protected:
    bool loadImage(QString imagePath);
    ImageNode *m_p;
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode to provide a static image or GIF
class ImageNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)

    friend class ImageNodeOpenGLWorker;

public:
    ImageNode();
    ImageNode(const ImageNode &other);
    ~ImageNode();

    // We don't actually need to do anything in paint(), because
    // periodic() advances the frame when necessary.  As a result,
    // there's no point in making a copy of the ImageNode before
    // paint() and copying it back afterwards.
    QSharedPointer<VideoNode> createCopyForRendering() override;
    void copyBackRenderState(QSharedPointer<Chain> chain, QSharedPointer<VideoNode> copy) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

public slots:
    QString imagePath();
    void setImagePath(QString imagePath);

protected slots:
    void onInitialized();
    void periodic();

signals:
    void imagePathChanged(QString imagePath);

protected:
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;

    QVector<GLuint> m_frameTextures;

    // We keep both the textre and texture idx to avoid having
    // to look up m_currentTextureIdx in m_frameTextures in texture()
    // which would involve taking out a lock to guard against
    // m_frameTextures being modified, and we need to take a lock
    // out in periodic() to increment m_ticksToNextFrame
    GLuint m_currentTexture;
    GLuint m_currentTextureIdx;

    QString m_imagePath;
    QSharedPointer<ImageNodeOpenGLWorker> m_openGLWorker;

    uint m_ticksToNextFrame;
    QTimer m_periodic;
    bool m_ready;
};
