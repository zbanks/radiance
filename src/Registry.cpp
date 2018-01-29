#include "Registry.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

#include "EffectNode.h"
#include "ImageNode.h"

Registry::Registry() {
    // This can be done with some black magic fuckery in the future
    m_factories.append(new EffectNodeFactory());
    m_factories.append(new ImageNodeFactory());
}

Registry::~Registry() {
    qDeleteAll(m_factories);
}

VideoNode *Registry::deserialize(Context *context, QString json) {
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject o = d.object();
    return deserialize(context, o);
}

VideoNode *Registry::deserialize(Context *context, QJsonObject object) {
    QString type = object.value("type").toString();
    if (type.isEmpty()) {
        qDebug() << "'type' not found in JSON";
        return nullptr;
    }
    for (auto f = m_factories.begin(); f != m_factories.end(); f++) {
        if ((*f)->typeName() == type) {
            return (*f)->deserialize(context, object);
        }
    }
    qDebug() << "Type" << type << "deserializer not found";
    return nullptr;
}

VideoNode *Registry::createFromFile(Context *context, QString filename) {
    for (auto f = m_factories.begin(); f != m_factories.end(); f++) {
        if ((*f)->canCreateFromFile(filename)) {
            return (*f)->fromFile(context, filename);
        }
    }
    qDebug() << "File handler not found";
    return nullptr;
}
