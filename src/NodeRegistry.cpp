#include "NodeRegistry.h"
#include "ProbeReg.h"

#include <QDir>
#include <QDebug>
#include <QRegularExpression>

NodeRegistry::NodeRegistry(QObject *p)
: QObject(p)
{
}
NodeRegistry::~NodeRegistry() = default;

VideoNode *NodeRegistry::createNode(const QString &nodeName) {
    auto arg = QString{};
    auto name = QString{};
    auto has_arg = false;
    Q_UNUSED(has_arg);

    auto colon = nodeName.indexOf(':');
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
    auto vnt = m_nodeTypes.value(name);
    if(!vnt)
        return nullptr;

    auto node = vnt->create(arg);
    return node;
}

QMap<QString, NodeType*> NodeRegistry::nodeTypes() {
    return m_nodeTypes;
}

QVariantMap NodeRegistry::qmlNodeTypes() {
    auto output = QVariantMap{};
    for (auto vnt : m_nodeTypes) {
        if(vnt) {
            output.insert(vnt->name(),QVariant::fromValue(vnt));
        }
    }
    return output;
}

void NodeRegistry::reload() {
    for(auto && vnt : m_nodeTypes.values()) {
        delete vnt;
    }
    m_nodeTypes.clear();

    auto nodeTypesList = ProbeRegistry::probeAll(this);
    for(auto vnt : nodeTypesList) {
        vnt->setParent(this);
        if(m_nodeTypes.contains(vnt->name()))
            delete vnt;
        else
            m_nodeTypes.insert(vnt->name(), vnt);
    }
    qInfo() << "Reloaded NodeRegistry:" << m_nodeTypes.size();
    emit nodeTypesChanged();
}
