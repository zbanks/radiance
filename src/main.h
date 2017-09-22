#ifndef __MAIN_H
#define __MAIN_H

#include <QSettings>
#include "UISettings.h"
#include "Audio.h"
#include "NodeRegistry.h"
#include "OpenGLWorkerContext.h"
#include "Timebase.h"
#include <QSharedPointer>
#include <QQmlApplicationEngine>

extern QSharedPointer<QSettings> settings;
extern QSharedPointer<UISettings> uiSettings;
extern QSharedPointer<Audio> audio;
extern QSharedPointer<NodeRegistry> nodeRegistry;
extern QSharedPointer<Timebase> timebase;
extern OpenGLWorkerContext *openGLWorkerContext;

#endif
