#include "Paths.h"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QtGlobal>

#ifndef RADIANCE_SYSTEM_RESOURCES
#error RADIANCE_SYSTEM_RESOURCES not set
#endif

bool Paths::m_initialized = false;
QString Paths::m_systemLibrary;
QString Paths::m_userLibrary;
QString Paths::m_systemConfig;
QString Paths::m_userConfig;
QString Paths::m_qml;
QString Paths::m_glsl;

QString Paths::systemLibrary() {
    Q_ASSERT(m_initialized);
    return Paths::m_systemLibrary;
}

QString Paths::userLibrary() {
    Q_ASSERT(m_initialized);
    return Paths::m_userLibrary;
}

QString Paths::systemConfig() {
    Q_ASSERT(m_initialized);
    return Paths::m_systemConfig;
}

QString Paths::userConfig() {
    Q_ASSERT(m_initialized);
    return Paths::m_userConfig;
}

QString Paths::qml() {
    Q_ASSERT(m_initialized);
    return Paths::m_qml;
}

QString Paths::glsl() {
    Q_ASSERT(m_initialized);
    return Paths::m_glsl;
}

void Paths::initialize() {
    QString systemResources(RADIANCE_SYSTEM_RESOURCES);
    if (!systemResources.startsWith("/")) {
        systemResources = QCoreApplication::applicationDirPath() + "/" + systemResources;
    }
    Paths::m_systemLibrary = systemResources + "library/";
    Paths::m_systemConfig = systemResources + "config/";
    Paths::m_qml = systemResources + "qml/";
    Paths::m_glsl = systemResources + "glsl/";

    auto appDataLocations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (appDataLocations.count() == 0) {
        qFatal("Could not determine AppDataLocation");
    }
    auto userPath = appDataLocations.at(0);
    Paths::m_userLibrary = userPath + "/library";
    Paths::m_userConfig = userPath + "/config";
    Paths::m_initialized = true;
    qDebug() << "System library path is:" << Paths::m_systemLibrary;
    qDebug() << "User library path is:" << Paths::m_userLibrary;
    qDebug() << "System config path is:" << Paths::m_systemConfig;
    qDebug() << "User config path is:" << Paths::m_userConfig;
    qDebug() << "QML path is:" << Paths::m_qml;
    qDebug() << "GLSL path is:" << Paths::m_glsl;
}

QString Paths::expandLibraryPath(QString filename) {
    Q_ASSERT(m_initialized);
    qDebug() << "Expanding library path:" << filename;
    if (QFileInfo(filename).isAbsolute()) {
        qDebug() << "Is absolute!";
        return filename;
    }
    auto userPath = QDir::cleanPath(userLibrary() + "/" + filename);
    if (QFileInfo(userPath).exists()) {
        qDebug() << "Is userpath!" << userPath;
        return userPath;
    }
    auto systemPath = QDir::cleanPath(systemLibrary() + "/" + filename);
    if (QFileInfo(systemPath).exists()) {
        qDebug() << "Is systempath!" << systemPath;
        return systemPath;
    }
    qDebug() << "Is not found! Returning userpath" << userPath;
    return userPath;
}

QString Paths::contractLibraryPath(QString filename) {
    Q_ASSERT(m_initialized);
    auto userLib = QDir(userLibrary());
    auto shortPath = userLib.relativeFilePath(filename);
    if (!shortPath.contains("..")) {
        return shortPath;
    }
    auto systemLib = QDir(systemLibrary());
    shortPath = systemLib.relativeFilePath(filename);
    if (!shortPath.contains("..")) {
        return shortPath;
    }
    return filename;
}

QString Paths::ensureUserLibrary(QString filename) {
    if (filename.startsWith("/")) return filename;
    auto p = QDir::cleanPath(userLibrary() + "/" + filename);
    QFileInfo(p).dir().mkpath(".");
    return p;
}

QString Paths::ensureUserConfig(QString filename) {
    if (filename.startsWith("/")) return filename;
    auto p = QDir::cleanPath(userConfig() + "/" + filename);
    QFileInfo(p).dir().mkpath(".");
    return p;
}
