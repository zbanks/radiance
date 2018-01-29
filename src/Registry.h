#pragma once

#include "VideoNode.h"
#include "VideoNodeFactory.h"

class Registry : public QObject {
public:
    Registry();
   ~Registry() override;

    VideoNode *deserialize(Context *context, QString json);
    VideoNode *deserialize(Context *context, QJsonObject object);
    VideoNode *createFromFile(Context *context, QString filename);

protected:
    QList<VideoNodeFactory*> m_factories;
};
