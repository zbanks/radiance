#include "Context.h"

#include "Audio.h"
#include "Timebase.h"
#include "OpenGLWorkerContext.h"
#include "Registry.h"

Context::Context(bool threaded) 
    : m_audio(nullptr)
    , m_timebase(nullptr)
    , m_openGLWorkerContext(nullptr)
{
    m_threaded = threaded;
    m_openGLWorkerContext = new OpenGLWorkerContext(threaded);

    m_timebase = new Timebase();
    m_audio = new Audio(m_timebase);
}

bool Context::threaded() {
    return m_threaded;
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
    delete m_audio;
    delete m_timebase;
}
