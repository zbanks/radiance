#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
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
protected slots:
    void onDestroyed();
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

    QString serialize();
    bool deserialize(const VideoNodeType &vnt, const QString &arg);
    static QList<VideoNodeType> availableNodeTypes();

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

    QVector<int> m_frameDelays; // milliseconds
    QVector<QOpenGLTexture *> m_frameTextures;
    QOpenGLTexture *m_currentTexture;
    int m_currentTextureIdx;

    QString m_imagePath;
    QSharedPointer<ImageNodeOpenGLWorker> m_openGLWorker;

    bool m_ready;
};
