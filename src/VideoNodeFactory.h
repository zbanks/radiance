#pragma once

#include <QObject>
#include "VideoNode.h"

// This is a pure abstract base class
// to aid in the creation of VideoNodes of all types.
// The Registry contains several VideoNodeFactories
// and dispatches to the appropriate one
// to instantiate VideoNodes.

class VideoNodeFactory {

public:
    // What type of VideoNode this VideoNodeFactory represents
    virtual QString typeName() = 0;

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    virtual VideoNode *deserialize(Context *context, QJsonObject obj) = 0;

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    virtual bool canCreateFromFile(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    virtual VideoNode *fromFile(Context *context, QString filename);
};
