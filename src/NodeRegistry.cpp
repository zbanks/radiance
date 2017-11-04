#include "NodeRegistry.h"
//#include "EffectNode.h"
//#include "ImageNode.h"
#include "ProbeReg.h"

//#ifdef USE_MPV
//#include "MovieNode.h"
//#endif

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

/*    if (!has_arg  && vnt.argRequired) {
        qInfo() << "Node type requires arg:" << name;
        return nullptr;
    }*/
    auto node = vnt->create(arg);
//    Q_ASSERT(m_videoNodeCreateFns.contains(vnt.className));
//    VideoNode * node = m_videoNodeCreateFns.value(vnt.className)();
//    bool rc = node->deserialize(vnt, arg);
//    Q_UNUSED(rc); // TODO: check rc

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
/*            auto entry = QVariantMap{};
            entry.insert("name", vnt->name());
    //        entry.insert("className", vnt.className);
            entry.insert("description", vnt->description());
            entry.insert("author", vnt->author());
            entry.insert("nInputs", vnt->inputCount());
            output.insert(vnt->name(), entry);*/
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
        if(m_nodeTypes.contains(vnt->name()))
            delete vnt;
        else
            m_nodeTypes.insert(vnt->name(), vnt);
    }
    qInfo() << "Reloaded NodeRegistry:" << m_nodeTypes.size();
    emit nodeTypesChanged();
}
