#include "Paths.h"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>

bool Paths::m_initialized = false;
QString Paths::m_library;
QString Paths::m_qml;
QString Paths::m_glsl;

QString Paths::library() {
    Q_ASSERT(m_initialized);
    return Paths::m_library;
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
    if (QDir(QCoreApplication::applicationDirPath() + "/../Resources/").exists()) {
        // MacOS Bundle
        Paths::m_library = QFileInfo(QCoreApplication::applicationDirPath() + "/../Resources/library/").absolutePath();
        Paths::m_qml = QFileInfo(QCoreApplication::applicationDirPath() + "/../Resources/qml/").absolutePath();
        Paths::m_glsl = QFileInfo(QCoreApplication::applicationDirPath() + "/../Resources/glsl/").absolutePath();
    } else if (QDir(QCoreApplication::applicationDirPath() + "/resources/").exists()) {
        // Linux Bundle
        Paths::m_library = QFileInfo(QCoreApplication::applicationDirPath() + "/resources/library/").absolutePath();
        Paths::m_qml = QFileInfo(QCoreApplication::applicationDirPath() + "/resources/qml/").absolutePath();
        Paths::m_glsl = QFileInfo(QCoreApplication::applicationDirPath() + "/resources/glsl/").absolutePath();
    } else if (QDir("../resources/").exists()) {
        // Debug build
        Paths::m_library = QFileInfo("../resources/library/").absolutePath();
        Paths::m_qml = QFileInfo("../resources/qml/").absolutePath();
        Paths::m_glsl = QFileInfo("../resources/glsl/").absolutePath();
    } else {
        // Anything else
        Paths::m_library = QFileInfo(QStandardPaths::locate(QStandardPaths::AppDataLocation, "library", QStandardPaths::LocateDirectory)).absolutePath();
        Paths::m_qml = QFileInfo(QStandardPaths::locate(QStandardPaths::AppDataLocation, "qml", QStandardPaths::LocateDirectory)).absolutePath();
        Paths::m_glsl = QFileInfo(QStandardPaths::locate(QStandardPaths::AppDataLocation, "glsl", QStandardPaths::LocateDirectory)).absolutePath();
    }
    Paths::m_initialized = true;
    qDebug() << "Library path is:" << Paths::m_library;
    qDebug() << "QML path is:" << Paths::m_qml;
    qDebug() << "GLSL path is:" << Paths::m_glsl;
}

QString Paths::expandLibraryPath(QString filename) {
    Q_ASSERT(m_initialized);
    if (QFileInfo(filename).isAbsolute()) {
        return filename;
    }
    auto fullPath = QDir::cleanPath(library() + "/" + filename);
    return fullPath;
}

QString Paths::contractLibraryPath(QString filename) {
    Q_ASSERT(m_initialized);
    auto lib = QDir(library());
    auto shortPath = lib.relativeFilePath(filename);
    if (shortPath.contains("..")) {
        return filename;
    } else {
        return shortPath;
    }
}
