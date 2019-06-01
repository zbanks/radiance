#pragma once

#include "VideoNode.h"

// This class extends VideoNode to passthrough its first input, but
// can be configured to be a wrapper for a different VideoNode

class PlaceholderNodePrivate;

class PlaceholderNode
    : public VideoNode {
    Q_OBJECT

public:
    PlaceholderNode(Context *context, VideoNodeSP *wrapped=nullptr);

    QJsonObject serialize() override;

    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;

    // These static methods are required for VideoNode creation
    // through the registry

    // A string representation of this VideoNode type
    static QString typeName();

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    static VideoNodeSP *deserialize(Context *context, QJsonObject obj);

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    static bool canCreateFromFile(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    static VideoNodeSP *fromFile(Context *context, QString filename);

    // Returns QML filenames that can be loaded
    // to instantiate custom instances of this VideoNode
    static QMap<QString, QString> customInstantiators();

public slots:
    void setWrappedVideoNode(VideoNodeSP *wrapped);
    VideoNodeSP *wrappedVideoNode();

    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;

signals:
    void wrappedVideoNodeChanged(VideoNodeSP *videoNode);

protected:
    // m_wrappedVideoNode needs to be a VideoNodeSP since there are QML properties that fetch it
    VideoNodeSP *m_wrappedVideoNode;
};

typedef QmlSharedPointer<PlaceholderNode, VideoNodeSP> PlaceholderNodeSP;
Q_DECLARE_METATYPE(PlaceholderNodeSP*)
