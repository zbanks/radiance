#include "Registry.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

#include "EffectNode.h"
#include "ImageNode.h"
#include "MovieNode.h"
#include "ScreenOutputNode.h"
#include "FfmpegOutputNode.h"
#include "PlaceholderNode.h"

Registry::Registry()
    : m_library(nullptr) {
    // This can be done with some black magic fuckery in the future
    registerType<EffectNode>();
    registerType<ImageNode>();
    registerType<MovieNode>();
    registerType<ScreenOutputNode>();
    registerType<FfmpegOutputNode>();
    registerType<PlaceholderNode>();

    m_library = new Library(this);
    m_library->setParent(this);
}

Registry::~Registry() {
}

template <class T> void Registry::registerType() {
    struct TypeFactory f{
        .typeName = T::typeName,
        .deserialize = T::deserialize,
        .canCreateFromFile = T::canCreateFromFile,
        .fromFile = T::fromFile,
    };
    m_factories.append(f);

    auto instantiators = T::customInstantiators();
    for (auto e = instantiators.begin(); e != instantiators.end(); e++) {
        m_instantiators.insert(e.key(), e.value());
    }
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
        if (f->typeName() == type) {
            return f->deserialize(context, object);
        }
    }
    qDebug() << "Type" << type << "deserializer not found";
    return nullptr;
}

bool Registry::canCreateFromFile(QString filename) {
    for (auto f = m_factories.begin(); f != m_factories.end(); f++) {
        if (f->canCreateFromFile(filename)) {
            return true;
        }
    }
    return false;
}

VideoNode *Registry::createFromFile(Context *context, QString filename) {
    for (auto f = m_factories.begin(); f != m_factories.end(); f++) {
        if (f->canCreateFromFile(filename)) {
            return f->fromFile(context, filename);
        }
    }
    qDebug() << "File handler not found";
    return nullptr;
}

Library *Registry::library() {
    return m_library;
}

const QMap<QString, QString> Registry::instantiators() const {
    return m_instantiators;
}
