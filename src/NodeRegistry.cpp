#include "NodeRegistry.h"
#include "EffectNode.h"
//#include "ImageNode.h"

#include <QDir>
#include <QDebug>

NodeRegistry::NodeRegistry() {
    reload();
}

NodeRegistry::~NodeRegistry() {

}

QSharedPointer<VideoNode> NodeRegistry::createNode(QString name) {
    if (!m_nodeTypes.contains(name))
        return nullptr;

    VideoNodeType vnt = m_nodeTypes.value(name);
    switch (vnt.type) {
    case VideoNodeType::EFFECT_NODE: {
        QSharedPointer<EffectNode> effect = QSharedPointer<EffectNode>(new EffectNode());
        effect->setName(name);
        effect->setInputCount(vnt.nInputs);
        return effect;
    }
// XXX
//    case VideoNodeType::IMAGE_NODE: {
//        QSharedPointer<ImageNode> image = QSharedPointer<ImageNode>(new ImageNode());
//        image->setImagePath(name);
//        image->setInputCount(vnt.nInputs);
//        return image;
//    }
    default:
        qInfo() << "Unknown type" << vnt.type;
        return nullptr;
    }
}

QHash<QString, VideoNodeType> NodeRegistry::nodeTypes() {
    return m_nodeTypes;
}

QVariantMap NodeRegistry::qmlNodeTypes() {
    QVariantMap output;
    //for (auto i = m_nodeTypes.begin(); i != m_nodeTypes.end(); i++) {
    for (auto vnt : m_nodeTypes) {
        QVariantMap entry;
        entry.insert("name", vnt.name);
        switch (vnt.type) {
        case VideoNodeType::EFFECT_NODE:
            entry.insert("type", "EffectNode");
            break;
        case VideoNodeType::IMAGE_NODE:
            entry.insert("type", "ImageNode");
            break;
        }
        entry.insert("description", vnt.description);
        entry.insert("nInputs", vnt.nInputs);
        output.insert(vnt.name, entry);
    }
    return output;
}

void NodeRegistry::reload() {
    m_nodeTypes.clear();

    auto filters = QStringList{} << QString{"*.0.glsl"};
    QDir dir("../resources/effects/");
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);

    for (auto effectName : dir.entryList()) {
        QString name = effectName.replace(".0.glsl", "");
        VideoNodeType nodeType = {
            .name = name,
            .type = VideoNodeType::EFFECT_NODE,
            .description = effectName,
            .nInputs = 1,
        };
        m_nodeTypes.insert(name, nodeType);
    }

    auto imgFilters = QStringList{} << QString{"*.gif"};
    QDir imgDir("../resources/images/");
    imgDir.setNameFilters(imgFilters);
    imgDir.setSorting(QDir::Name);

    for (auto imageName : imgDir.entryList()) {
        QString name = imageName;
        VideoNodeType nodeType = {
            .name = name,
            .type = VideoNodeType::IMAGE_NODE,
            .description = imageName,
            .nInputs = 1, // TODO: This should be 0 but it breaks the UI
        };
        m_nodeTypes.insert(name, nodeType);
    }

    emit nodeTypesChanged();
}
