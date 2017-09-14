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

public slots:
    QHash<QString, VideoNodeType> nodeTypes();
    QVariantMap qmlNodeTypes();

    void reload();

signals:
    void nodeTypesChanged();

private:
    QHash<QString, VideoNodeType> m_nodeTypes;
};

