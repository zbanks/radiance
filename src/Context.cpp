#include "Context.h"

#include "Audio.h"
#include "Timebase.h"
#include "OpenGLWorkerContext.h"
#include "Registry.h"

#include <QSettings>

Context::Context(bool threaded) 
    : m_settings(nullptr)
    , m_audio(nullptr)
    , m_timebase(nullptr)
    , m_openGLWorkerContext(nullptr)
{
    m_openGLWorkerContext = new OpenGLWorkerContext(threaded);

    m_settings = new QSettings();
    //m_outputSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Radiance", "Radiance Output");
    m_timebase = new Timebase();
    m_audio = new Audio(m_timebase);
}

QSettings *Context::settings() {
    return m_settings;
}

Audio *Context::audio() {
    return m_audio;
}

Timebase *Context::timebase() {
    return m_timebase;
}

OpenGLWorkerContext *Context::openGLWorkerContext() {
    return m_openGLWorkerContext;
}

Context::~Context() {
    delete m_settings;
    delete m_audio;
    delete m_timebase;
    //delete m_outputSettings;
}
