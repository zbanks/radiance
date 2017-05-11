#ifndef __MAIN_H
#define __MAIN_H

#include <QSettings>
#include "UISettings.h"
#include "RenderContext.h"
#include "Audio.h"
#include "Timebase.h"
#include <utility>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <numeric>

extern RenderContext *renderContext;
extern QSettings *settings;
extern UISettings *uiSettings;
extern Audio *audio;
extern Timebase *timebase;

#endif
