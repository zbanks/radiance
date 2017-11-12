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
        Paths::m_library = QCoreApplication::applicationDirPath() + "/../Resources/library/";
        Paths::m_qml = QCoreApplication::applicationDirPath() + "/../Resources/qml/";
        Paths::m_glsl = QCoreApplication::applicationDirPath() + "/../Resources/glsl/";
    } else if (QDir(QCoreApplication::applicationDirPath() + "/resources/").exists()) {
        // Linux Bundle
        Paths::m_library = QCoreApplication::applicationDirPath() + "/resources/library/";
        Paths::m_qml = QCoreApplication::applicationDirPath() + "/resources/qml/";
        Paths::m_glsl = QCoreApplication::applicationDirPath() + "/resources/glsl/";
    } else if (QDir("../resources/").exists()) {
        // Debug build
        Paths::m_library = "../resources/library/";
        Paths::m_qml = "../resources/qml/";
        Paths::m_glsl = "../resources/glsl/";
    } else {
        // Anything else
        Paths::m_library = QStandardPaths::locate(QStandardPaths::AppDataLocation, "library", QStandardPaths::LocateDirectory);
        Paths::m_qml = QStandardPaths::locate(QStandardPaths::AppDataLocation, "qml", QStandardPaths::LocateDirectory);
        Paths::m_glsl = QStandardPaths::locate(QStandardPaths::AppDataLocation, "glsl", QStandardPaths::LocateDirectory);
    }
    Paths::m_initialized = true;
    qDebug() << "Library path is:" << Paths::m_library;
    qDebug() << "QML path is:" << Paths::m_qml;
    qDebug() << "GLSL path is:" << Paths::m_glsl;
}
