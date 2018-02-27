#pragma once

#include "VideoNode.h"

// This class extends VideoNode to passthrough its first input, but
// can be configured to be a wrapper for a different VideoNode

class PlaceholderNode
    : public VideoNode {
    Q_OBJECT

public:
    PlaceholderNode(Context *context, VideoNode *wrapped=nullptr);
    PlaceholderNode(const PlaceholderNode &other, QSharedPointer<VideoNode> ownedWrapped=nullptr);
    ~PlaceholderNode();

    QJsonObject serialize() override;

    GLuint paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) override;
    // Creates a copy of this node
    QSharedPointer<VideoNode> createCopyForRendering(QSharedPointer<Chain>) override;

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
    void setWrappedVideoNode(VideoNode *wrapped);
    void chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) override;

signals:

protected:
    VideoNode *m_wrappedVideoNode;
    QSharedPointer<VideoNode> m_ownedWrappedVideoNode;
};
