#include "VideoNodeFactory.h"

bool VideoNodeFactory::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *VideoNodeFactory::fromFile(Context *context, QString filename) {
    return nullptr;
}

