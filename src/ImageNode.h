#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <vector>

class ImageNode;

// This class extends OpenGLWorker
// to enable shader compilation
// and other initialization
// in a background context
class ImageNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    ImageNodeOpenGLWorker(ImageNode *p, QString file);
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
    bool loadImage(QString file);
    ImageNode *m_p;
protected slots:
    void onDestroyed();
public:
    std::atomic<bool> m_ready{false};
    QString      m_file;
    int          m_totalDelay{};
    std::vector<int> m_frameDelays{}; // milliseconds
    std::vector<QSharedPointer<QOpenGLTexture>> m_frameTextures{};
};

///////////////////////////////////////////////////////////////////////////////

// This class extends VideoNode to provide a static image or GIF
class ImageNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

    friend class ImageNodeOpenGLWorker;

public:
    ImageNode(Context *context, QString file);
    ImageNode(const ImageNode &other);
    ~ImageNode();

    QJsonObject serialize() override;

    // We don't actually need to do anything in paint(), because
    // periodic() advances the frame when necessary.  As a result,
    // there's no point in making a copy of the ImageNode before
    // paint() and copying it back afterwards.
    QSharedPointer<VideoNode> createCopyForRendering(QSharedPointer<Chain>) override;
    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

    // These static methods are required for VideoNode creation
    // through the registry

    // A string representation of this VideoNode type
    static QString typeName();

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    static VideoNode *deserialize(Context *context, QJsonObject obj);

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    static bool canCreateFromFile(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    static VideoNode *fromFile(Context *context, QString filename);

public slots:
    QString file();
    QString name();
    void setFile(QString file);

protected slots:
    void onInitialized();

signals:
    void fileChanged(QString file);
    void nameChanged(QString name);

protected:
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;

    QString m_file;
    QSharedPointer<ImageNodeOpenGLWorker> m_openGLWorker;

    bool m_ready;
};
