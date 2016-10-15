#ifndef __MAIN_H
#define __MAIN_H

#include <QThread>
#include <QSettings>
#include "UISettings.h"

extern QThread *renderThread;
extern QSettings *settings;
extern UISettings *uiSettings;

#endif
