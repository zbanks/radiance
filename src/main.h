#ifndef __MAIN_H
#define __MAIN_H

#include <QSettings>
#include "UISettings.h"
#include "RenderContextOld.h"
#include "Audio.h"
#include "Timebase.h"
#include "RenderContext.h"
#include "OpenGLWorkerContext.h"
#include <QSharedPointer>
#include <QQmlApplicationEngine>

extern RenderContext *renderContext;
extern RenderContextOld *renderContextOld;
extern QSettings *settings;
extern UISettings *uiSettings;
extern Audio *audio;
extern Timebase *timebase;
extern OpenGLWorkerContext *openGLWorkerContext;

#endif
