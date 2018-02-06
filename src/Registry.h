#pragma once

#include "VideoNode.h"
#include "Library.h"
#include <QAbstractItemModel>

struct TypeFactory {
    // What type of VideoNode this VideoNodeFactory represents
    QString (*typeName)();

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    VideoNode *(*deserialize)(Context *context, QJsonObject obj);

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    bool (*canCreateFromFile)(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    VideoNode *(*fromFile)(Context *context, QString filename);
};

class Registry : public QObject {
    Q_OBJECT

public:
    Registry();
   ~Registry() override;
    template <class T> void registerType();

    Q_PROPERTY(Library *library READ library CONSTANT);

public slots:
    VideoNode *deserialize(Context *context, QString json);
    VideoNode *deserialize(Context *context, QJsonObject object);
    VideoNode *createFromFile(Context *context, QString filename);

    Library *library();

protected:
    QList<TypeFactory> m_factories;
    Library *m_library;
};
