#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "EffectUI.h"
#include "CrossFaderUI.h"
#include "OutputUI.h"
#include "GraphicalDisplayUI.h"
#include "RenderContext.h"
#include "Output.h"
#include "Lux.h"
#include "main.h"

RenderContext *renderContext = 0;
QSettings *settings = 0;
QSettings *outputSettings = 0;
UISettings *uiSettings = 0;
Audio *audio = 0;
OutputManager *outputManager = 0;
EffectList *effectList = 0;
Timebase *timebase = 0;

QObject *uiSettingsProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return uiSettings;
}

QObject *audioProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return audio;
}

QObject *outputManagerProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return outputManager;
}

QObject *effectListProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return effectList;
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);
    //qRegisterMetaType<Effect*>("Effect*");

    settings = new QSettings();
    outputSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Radiance", "Radiance Output");
    uiSettings = new UISettings();
    audio = new Audio();
    outputManager = new OutputManager(outputSettings);
    timebase = new Timebase();
    effectList = new EffectList();

    qmlRegisterUncreatableType<VideoNodeUI>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterType<EffectUI>("radiance", 1, 0, "Effect");
    qmlRegisterType<CrossFaderUI>("radiance", 1, 0, "CrossFader");
    qmlRegisterType<OutputUI>("radiance", 1, 0, "Output");
    qmlRegisterType<GraphicalDisplayUI>("radiance", 1, 0, "GraphicalDisplay");

    qmlRegisterSingletonType<UISettings>("radiance", 1, 0, "UISettings", uiSettingsProvider);
    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);
    qmlRegisterSingletonType<EffectList>("radiance", 1, 0, "EffectList", effectListProvider);

    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<LuxDevice>("radiance", 1, 0, "LuxDevice");
    qmlRegisterSingletonType<OutputManager>("radiance", 1, 0, "OutputManager", outputManagerProvider);

    // Render context
    QThread renderThread;
    renderThread.setObjectName("RenderThread");
    renderContext = new RenderContext();
    renderContext->moveToThread(&renderThread);
    QObject::connect(&renderThread, &QThread::started, renderContext, &RenderContext::start);
    renderThread.start();

    QQmlApplicationEngine engine(QUrl("../resources/qml/application.qml"));
    if(engine.rootObjects().isEmpty()) {
        qFatal("Failed to load main QML application");
        return 1;
    }

    QObject *window = engine.rootObjects().first();
    renderContext->addSyncSource(window);

    return app.exec();
}
