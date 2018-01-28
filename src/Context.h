#pragma once

// A radiance context encapsulates
// all of the background stuff that goes on
// that allows VideoNodes to function as they do.
// It includes:
// * Audio analysis
// * Timebase
// * A worker OpenGL context

#include <QObject>

class QSettings;
class Audio;
class Timebase;
class OpenGLWorkerContext;

class Context : public QObject {
public:
    Context();
   ~Context();

    QSettings *settings();
    Audio *audio();
    Timebase *timebase();
    OpenGLWorkerContext *openGLWorkerContext();

protected:
    QSettings *m_settings;
    Audio *m_audio;
    Timebase *m_timebase;
    OpenGLWorkerContext *m_openGLWorkerContext;
};
