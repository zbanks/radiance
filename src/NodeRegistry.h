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

/*    template <typename VN>
    void registerVideoNodeSubclass() {
        Q_ASSERT(VN::staticMetaObject.inherits(&VideoNode::staticMetaObject));
        QString className = QString(VN::staticMetaObject.className());
        m_videoNodeCreateFns.insert(className, &videoNodeCreate<VN>);
        m_videoNodeListTypesFns.insert(className, &videoNodeListTypes<VN>);
    }*/

public slots:
    QMap<QString, NodeType *> nodeTypes();
    QVariantMap qmlNodeTypes();

    void reload();

signals:
    void nodeTypesChanged();

private:
    QMap<QString, NodeType*> m_nodeTypes;
//    QMap<QString, VideoNodeType> m_nodeTypes;

/*    QMap<QString, VideoNode* (*)()> m_videoNodeCreateFns;
    template <typename VN>
    static VideoNode* videoNodeCreate() {
        return new VN();
    }

    QMap<QString, QList<VideoNodeType> (*)()> m_videoNodeListTypesFns;
    template <typename VN>
    static QList<VideoNodeType> videoNodeListTypes() {
        return VN::availableNodeTypes();
    }*/
};

