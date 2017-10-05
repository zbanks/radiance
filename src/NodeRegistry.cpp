#include "NodeRegistry.h"
#include "EffectNode.h"
#include "ImageNode.h"
#include "MovieNode.h"

#include <QDir>
#include <QDebug>
#include <QRegularExpression>

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

    if (has_arg && name == "youtube") {
        MovieNode *movie = new MovieNode();
        movie->setVideoPath(QString("ytdl://ytsearch:%1").arg(arg));
        return movie;
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
    case VideoNodeType::MOVIE_NODE: {
        MovieNode *movie = new MovieNode();
        movie->setVideoPath(name);
        return movie;
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

    MovieNode * movieNode = qobject_cast<MovieNode *>(node);
    if (movieNode) {
        return movieNode->videoPath();
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
        case VideoNodeType::MOVIE_NODE:
            entry.insert("type", "MovieNode");
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

    auto filters = QStringList{} << QString{"*.glsl"};
    QDir dir("../resources/effects/");
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);

    for (auto effectName : dir.entryList()) {
        auto name = effectName.replace(".glsl", "");
        auto filename = QString("../resources/effects/%1.glsl").arg(name);
        VideoNodeType nodeType = {
            .name = name,
            .type = VideoNodeType::EFFECT_NODE,
            .description = effectName,
            .nInputs = 1,
        };
        QFileInfo check_file(filename);
        if(!(check_file.exists() && check_file.isFile()))
            continue;

        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        QTextStream stream(&file);

        auto buffershader_reg = QRegularExpression(
            "^\\s*#buffershader\\s*$"
        , QRegularExpression::CaseInsensitiveOption
            );
        auto property_reg = QRegularExpression(
            "^\\s*#property\\s+(?<name>\\w+)\\s+(?<value>.*)$"
        , QRegularExpression::CaseInsensitiveOption
            );
        auto passes = QVector<QStringList>{QStringList{"#line 0"}};
        auto props  = QMap<QString,QString>{{"inputCount","1"}};
        auto lineno = 0;
        for(auto next_line = QString{}; stream.readLineInto(&next_line);++lineno) {
            {
                auto m = property_reg.match(next_line);
                if(m.hasMatch()) {
                    props.insert(m.captured("name"),m.captured("value"));
                    passes.back().append(QString{"#line %1"}.arg(lineno));
                    qDebug() << "setting property " << m.captured("name") << " to value " << m.captured("value");
                    continue;
                }
            }
            {
                auto m = buffershader_reg.match(next_line);
                if(m.hasMatch()) {
                    passes.append({QString{"#line %1"}.arg(lineno)});
                    continue;
                }
            }
            passes.back().append(next_line);
        }
        nodeType.nInputs = QVariant::fromValue(props["inputCount"]).value<int>();
        // TODO: Get this information in a real way
/*        if (name == "greenscreen" ||
            name == "crossfader" ||
            name == "uvmap" ||
            name == "composite") {
            nodeType.nInputs = 2;
        } else if (name == "rgbmask") {
            nodeType.nInputs = 4;
        }*/
        //qInfo() << name << nodeType.name;

        m_nodeTypes.insert(name, nodeType);
    }

    QDir imgDir("../resources/images/");
    imgDir.setSorting(QDir::Name);

    for (auto imageName : imgDir.entryList()) {
        if (imageName[0] == '.')
            continue;

        QString name = imageName;
        VideoNodeType nodeType = {
            .name = name,
            .type = VideoNodeType::IMAGE_NODE,
            .description = imageName,
            .nInputs = 1, // TODO: This should be 0 but it breaks the UI
        };
        m_nodeTypes.insert(name, nodeType);
    }

    QDir movieDir("../resources/videos/");
    movieDir.setSorting(QDir::Name);

    for (auto movieName : movieDir.entryList()) {
        if (movieName[0] == '.')
            continue;

        QString name = movieName;
        VideoNodeType nodeType = {
            .name = name,
            .type = VideoNodeType::MOVIE_NODE,
            .description = movieName,
            .nInputs = 1, // TODO: This should be 0 but it breaks the UI
        };
        m_nodeTypes.insert(name, nodeType);
    }

    qInfo() << "Reloaded NodeRegistry:" << m_nodeTypes.size();
    emit nodeTypesChanged();
}
