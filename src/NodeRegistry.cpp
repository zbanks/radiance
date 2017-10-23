#include "NodeRegistry.h"
#include "EffectNode.h"
#include "ImageNode.h"

#ifdef USE_MPV
#include "MovieNode.h"
#endif

#include <QDir>
#include <QDebug>
#include <QRegularExpression>

NodeRegistry::NodeRegistry() {
}

NodeRegistry::~NodeRegistry() {

}

VideoNode *NodeRegistry::createNode(const QString &nodeName) {
    QString arg, name;
    bool has_arg = false;

    int colon = nodeName.indexOf(':');
    if (colon >= 0) {
        arg = nodeName.mid(colon + 1);
        name = nodeName.left(colon);
        has_arg = true;
    } else {
        name = nodeName;
    }

    if (!m_nodeTypes.contains(name)) {
        qInfo() << "Unknown node type:" << name;
        return nullptr;
    }

    VideoNodeType vnt = m_nodeTypes.value(name);
    if (!has_arg  && vnt.argRequired) {
        qInfo() << "Node type requires arg:" << name;
        return nullptr;
    }

    Q_ASSERT(m_videoNodeCreateFns.contains(vnt.className));
    VideoNode * node = m_videoNodeCreateFns.value(vnt.className)();
    bool rc = node->deserialize(vnt, arg);
    // TODO: check rc
    
    return node;
}

QMap<QString, VideoNodeType> NodeRegistry::nodeTypes() {
    return m_nodeTypes;
}

QVariantMap NodeRegistry::qmlNodeTypes() {
    QVariantMap output;
    for (auto vnt : m_nodeTypes) {
        QVariantMap entry;
        entry.insert("name", vnt.name);
        entry.insert("className", vnt.className);
        entry.insert("description", vnt.description);
        entry.insert("author", vnt.author);
        entry.insert("nInputs", vnt.nInputs);
        output.insert(vnt.name, entry);
    }
    return output;
}

void NodeRegistry::reload() {
    m_nodeTypes.clear();

    auto it = m_videoNodeListTypesFns.constBegin();
    while (it != m_videoNodeListTypesFns.constEnd()) {
        QList<VideoNodeType> types = it.value()();
        for (auto type : types) {
            type.className = it.key();
            m_nodeTypes.insert(type.name, type);
        }
        it++;
    }

    qInfo() << "Reloaded NodeRegistry:" << m_nodeTypes.size();
    emit nodeTypesChanged();
}
