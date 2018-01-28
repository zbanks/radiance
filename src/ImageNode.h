#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <vector>

class ImageNode;

///////////////////////////////////////////////////////////////////////////////

// This class extends OpenGLWorker
// to enable shader compilation
// and other initialization
// in a background context
class ImageNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    ImageNodeOpenGLWorker(ImageNode *p, QString imagePath);
    ~ImageNodeOpenGLWorker() override;

public slots:
    void initialize();
    bool ready() const;
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
public:
    std::atomic<bool> m_ready{false};
    QString      m_imagePath;
    int          m_totalDelay{};
    std::vector<int> m_frameDelays{}; // milliseconds
    std::vector<QSharedPointer<QOpenGLTexture>> m_frameTextures{};
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode to provide a static image or GIF
class ImageNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)

    friend class ImageNodeOpenGLWorker;

public:
    ImageNode(NodeType *nr);
    ImageNode(const ImageNode &other);
    ~ImageNode();

    QString serialize() override;

    // We don't actually need to do anything in paint(), because
    // periodic() advances the frame when necessary.  As a result,
    // there's no point in making a copy of the ImageNode before
    // paint() and copying it back afterwards.
    QSharedPointer<VideoNode> createCopyForRendering(QSharedPointer<Chain>) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

public slots:
    QString imagePath();
    void setImagePath(QString imagePath);

protected slots:
    void onInitialized();

signals:
    void imagePathChanged(QString imagePath);

protected:
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;


    QString m_imagePath;
    QSharedPointer<ImageNodeOpenGLWorker> m_openGLWorker;

    bool m_ready;
};
