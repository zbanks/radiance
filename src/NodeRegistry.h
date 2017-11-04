#pragma once

#include "VideoNode.h"
#include "NodeType.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QSharedPointer>

class NodeRegistry : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap nodeTypes READ qmlNodeTypes NOTIFY nodeTypesChanged)
public:
    NodeRegistry( QObject *p = nullptr);
    ~NodeRegistry() override;

    // Create node from string; resulting VideoNode is caller-owned
    VideoNode *createNode(const QString &name);

public slots:
    QMap<QString, NodeType *> nodeTypes();
    QVariantMap qmlNodeTypes();

    void reload();

signals:
    void nodeTypesChanged();

private:
    QMap<QString, NodeType*> m_nodeTypes;
};

