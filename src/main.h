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

extern QSharedPointer<RenderContext> renderContext;
extern QSharedPointer<RenderContextOld> renderContextOld;
extern QSharedPointer<QSettings> settings;
extern QSharedPointer<UISettings> uiSettings;
extern QSharedPointer<Audio> audio;
extern QSharedPointer<Timebase> timebase;
extern QSharedPointer<OpenGLWorkerContext> openGLWorkerContext;

#endif
