#pragma once

// A radiance context encapsulates
// all of the background stuff that goes on
// that allows VideoNodes to function as they do.

#include "VideoNode.h"

#include <QObject>

class QSettings;
class Audio;
class Timebase;
class OpenGLWorkerContext;

class Context : public QObject {
    Q_OBJECT

public:
    Context(bool threaded=true);
   ~Context();

    Audio *audio();
    Timebase *timebase();
    OpenGLWorkerContext *openGLWorkerContext();

protected:
    Audio *m_audio;
    Timebase *m_timebase;
    OpenGLWorkerContext *m_openGLWorkerContext;
};
