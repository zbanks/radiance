#pragma once
#include <QString>

class Paths {
public:
    static void initialize();
    static QString library();
    static QString qml();
    static QString glsl();
    static QString models();

    static QString expandLibraryPath(QString filename);
    static QString contractLibraryPath(QString filename);

private:
    static QString m_library;
    static QString m_qml;
    static QString m_glsl;
    static QString m_models;
    static bool m_initialized;
};
