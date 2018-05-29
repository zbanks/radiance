#pragma once

#include "SelfTimedReadBackOutputNode.h"

class LuxOutputNodePrivate;

class LuxOutputNode
    : public SelfTimedReadBackOutputNode {
    Q_OBJECT

public:
    LuxOutputNode(Context *context, QSize chainSize);
    LuxOutputNode(const LuxOutputNode &other);
    LuxOutputNode *clone() const override;
    void onFrame(QSize size, QByteArray frame);

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

private:
    LuxOutputNode(QSharedPointer<LuxOutputNodePrivate> other_ptr);
    QSharedPointer<LuxOutputNodePrivate> d() const;
};

class LuxOutputNodePrivate : public SelfTimedReadBackOutputNodePrivate {
public:
    LuxOutputNodePrivate(Context *context, QSize chainSize);
};
