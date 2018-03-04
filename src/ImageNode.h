#pragma once

#include "VideoNode.h"
#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QMutex>
#include <QTimer>
#include <vector>

class ImageNodeOpenGLWorker;
class ImageNodePrivate;

// This class extends VideoNode to provide a static image or GIF
class ImageNode
    : public VideoNode {
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

    friend class WeakImageNode;
    friend class ImageNodeOpenGLWorker;

public:
    ImageNode(Context *context, QString file);
    ImageNode(const ImageNode &other);
    ImageNode *clone() const override;

    QJsonObject serialize() override;

    // We don't actually need to do anything in paint(), because
    // periodic() advances the frame when necessary.  As a result,
    // there's no point in making a copy of the ImageNode before
    // paint() and copying it back afterwards.
    GLuint paint(Chain chain, QVector<GLuint> inputTextures) override;

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

    // Returns QML filenames that can be loaded
    // to instantiate custom instances of this VideoNode
    static QMap<QString, QString> customInstantiators();

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
    void chainsEdited(QList<Chain> added, QList<Chain> removed) override;

private:
    ImageNode(QSharedPointer<ImageNodePrivate> other_ptr);
    QSharedPointer<ImageNodePrivate> d() const;
    QString fileToName();
};

class ImageNodePrivate : public VideoNodePrivate {
public:
    ImageNodePrivate(Context *context);

    QString m_file;

    // This is not actually shared,
    // I just like the deletion semantics
    QSharedPointer<ImageNodeOpenGLWorker> m_openGLWorker{};

    bool m_ready{false};

    int          m_totalDelay{};
    QVector<int> m_frameDelays{}; // milliseconds
    QVector<QSharedPointer<QOpenGLTexture>> m_frameTextures{};
};

///////////////////////////////////////////////////////////////////////////////

class WeakImageNode {
public:
    WeakImageNode();
    WeakImageNode(const ImageNode &other);
    QSharedPointer<ImageNodePrivate> toStrongRef();

protected:
    QWeakPointer<ImageNodePrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////

// This class extends OpenGLWorker
// to enable shader compilation
// and other initialization
// in a background context
class ImageNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT

public:
    ImageNodeOpenGLWorker(ImageNode p);

public slots:
    void initialize(QString filename);

signals:
    void message(QString str);
    void warning(QString str);
    void fatal(QString str);
protected:
    WeakImageNode m_p;
};
