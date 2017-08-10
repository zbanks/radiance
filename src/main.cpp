#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "CrossFaderUI.h"
#include "EffectUI.h"
#include "GraphicalDisplayUI.h"
#include "Lux.h"
#include "Midi.h"
#include "Output.h"
#include "OutputUI.h"
#include "RenderContextOld.h"
#include "VideoNode.h"
#include "QQuickVideoNodeRender.h"
#include "Model.h"
#include "EffectNode.h"
#include "main.h"

RenderContextOld *renderContextOld = 0;
RenderContext *renderContext = 0;
OpenGLWorkerContext *openGLWorkerContext = 0;
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

QObject *renderContextProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return renderContext;
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);
    //qRegisterMetaType<Effect*>("Effect*");

    QThread::currentThread()->setObjectName("mainThread");

    openGLWorkerContext = new OpenGLWorkerContext();
    openGLWorkerContext->setObjectName("openGLWorkerContext");
    openGLWorkerContext->start();

    settings = new QSettings();
    outputSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Radiance", "Radiance Output");
    uiSettings = new UISettings();
    audio = new Audio();
    outputManager = new OutputManager(outputSettings);
    timebase = new Timebase();
    effectList = new EffectList();
    renderContext = new RenderContext();

    qmlRegisterUncreatableType<VideoNode>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterType<Model>("radiance", 1, 0, "Model");
    qmlRegisterUncreatableType<ModelGraph>("radiance", 1, 0, "ModelGraph", "ModelGraph must be retrieved using Model.graph");
    qmlRegisterType<EffectNode>("radiance", 1, 0, "EffectNode");

    qmlRegisterUncreatableType<VideoNodeOld>("radiance", 1, 0, "VideoNodeOld", "VideoNodeOld is abstract and cannot be instantiated");
    qmlRegisterUncreatableType<VideoNodeUI>("radiance", 1, 0, "VideoNodeUI", "VideoNodeUI is abstract and cannot be instantiated");
    qmlRegisterType<EffectUI>("radiance", 1, 0, "Effect");
    qmlRegisterType<QQuickVideoNodeRender>("radiance", 1, 0, "VideoNodeRender");
    qmlRegisterType<CrossFaderUI>("radiance", 1, 0, "CrossFader");
    qmlRegisterType<OutputUI>("radiance", 1, 0, "Output");
    qmlRegisterType<GraphicalDisplayUI>("radiance", 1, 0, "GraphicalDisplay");
    qmlRegisterType<MidiDevice>("radiance", 1, 0, "MidiDevice");

    qmlRegisterSingletonType<UISettings>("radiance", 1, 0, "UISettings", uiSettingsProvider);
    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);
    qmlRegisterSingletonType<EffectList>("radiance", 1, 0, "EffectList", effectListProvider);
    qmlRegisterSingletonType<RenderContext>("radiance", 1, 0, "RenderContext", renderContextProvider);

    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<LuxDevice>("radiance", 1, 0, "LuxDevice");
    qmlRegisterSingletonType<OutputManager>("radiance", 1, 0, "OutputManager", outputManagerProvider);

    // Render context
    QThread renderThread;
    renderThread.setObjectName("RenderThread");
    renderContextOld = new RenderContextOld();
    renderContextOld->moveToThread(&renderThread);
    QObject::connect(&renderThread, &QThread::started, renderContextOld, &RenderContextOld::start);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &renderThread, &QThread::quit);
    renderThread.start();

    QQmlApplicationEngine engine(QUrl("../resources/qml/application.qml"));
    if(engine.rootObjects().isEmpty()) {
        qFatal("Failed to load main QML application");
        return 1;
    }

    QObject *window = engine.rootObjects().first();
    renderContextOld->addSyncSource(window);

    return app.exec();
}
