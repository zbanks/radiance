#pragma once
#include <QString>

class Paths {
public:
    static void initialize();
    static QString systemLibrary();
    static QString userLibrary();
    static QString systemConfig();
    static QString userConfig();
    static QString qml();
    static QString glsl();

    static QString expandLibraryPath(QString filename);
    static QString contractLibraryPath(QString filename);

    // These functions take in a path.
    // If the path is absolute, it is returned untouched.
    // If that path is relative, it is assumed to be
    // in the user directory and an absolute path is returned.
    // All necessary directories under it will be created,
    // allowing you to safely write the file.
    static QString ensureUserLibrary(QString filename);
    static QString ensureUserConfig(QString filename);

private:
    static QString m_systemLibrary;
    static QString m_userLibrary;
    static QString m_systemConfig;
    static QString m_userConfig;
    static QString m_qml;
    static QString m_glsl;
    static bool m_initialized;
};
