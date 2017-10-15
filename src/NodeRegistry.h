#pragma once

#include "VideoNode.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QSharedPointer>

struct VideoNodeType {
    QString name;
    enum {
        EFFECT_NODE,
        IMAGE_NODE,
#if MPV_FOUND == TRUE
        MOVIE_NODE,
#endif
    } type;
    QString description;
    int nInputs;
};

class NodeRegistry : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap nodeTypes READ qmlNodeTypes NOTIFY nodeTypesChanged)

public:
    NodeRegistry();
    ~NodeRegistry() override;

    // Create node from string; resulting VideoNode is caller-owned
    VideoNode *createNode(const QString &name);
    // Inverse of `createNode`
    QString serializeNode(VideoNode *node);

public slots:
    QMap<QString, VideoNodeType> nodeTypes();
    QVariantMap qmlNodeTypes();

    void reload();

signals:
    void nodeTypesChanged();

private:
    QMap<QString, VideoNodeType> m_nodeTypes;
};

