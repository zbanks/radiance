#include "NodeRegistry.h"
#include "EffectNode.h"
#include "ImageNode.h"

#include <QDir>
#include <QDebug>

NodeRegistry::NodeRegistry() {
    reload();
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
    switch (vnt.type) {
    case VideoNodeType::EFFECT_NODE: {
        EffectNode *effect = new EffectNode();
        effect->setName(name);
        effect->setInputCount(vnt.nInputs);
        if (has_arg) {
            effect->setIntensity(arg.toFloat());
        }
        return effect;
    }
    case VideoNodeType::IMAGE_NODE: {
        ImageNode *image = new ImageNode();
        image->setImagePath(name);
        image->setInputCount(vnt.nInputs);
        return image;
    }
    default:
        qInfo() << "Unknown type:" << vnt.type;
        return nullptr;
    }
}

QString NodeRegistry::serializeNode(VideoNode *node) {
    //TODO: this should use JSON and delegate to each VideoNode type
    EffectNode * effectNode = qobject_cast<EffectNode *>(node);
    if (effectNode) {
        return QString("%1:%2").arg(effectNode->name(), QString::number(effectNode->intensity()));
    }

    ImageNode * imageNode = qobject_cast<ImageNode *>(node);
    if (imageNode) {
        return imageNode->imagePath();
    }

    qInfo() << "Unable to serialize unknown node:" << node;
    return QString("unknown");
}

QMap<QString, VideoNodeType> NodeRegistry::nodeTypes() {
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

        // TODO: Get this information in a real way
        if (name == "greenscreen" ||
            name == "crossfader" ||
            name == "composite") {
            nodeType.nInputs = 2;
        } else if (name == "rgbmask") {
            nodeType.nInputs = 4;
        }
        //qInfo() << name << nodeType.name;

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

    qInfo() << "Reloaded NodeRegistry:" << m_nodeTypes.size();
    emit nodeTypesChanged();
}
