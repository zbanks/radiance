#pragma once

#include "SelfTimedReadBackOutputNode.h"

class ConsoleOutputNode
    : public SelfTimedReadBackOutputNode {
    Q_OBJECT

public:
    ConsoleOutputNode(Context *context, QSize chainSize);
    ConsoleOutputNode(const ConsoleOutputNode &other);
    ~ConsoleOutputNode();
    void frame(QSize size, QByteArray frame) override;

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
};
