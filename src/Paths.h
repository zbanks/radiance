#pragma once
#include <QString>

class Paths {
public:
    static void initialize(bool debug=false);
    static QString library();
    static QString qml();
    static QString glsl();
private:
    static QString m_library;
    static QString m_qml;
    static QString m_glsl;
    static bool m_initialized;
};
