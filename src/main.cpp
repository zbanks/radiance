#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "Lux.h"
#include "Midi.h"
#include "Output.h"
#include "VideoNode.h"
#include "QQuickVideoNodeRender.h"
#include "Model.h"
#include "View.h"
#include "EffectNode.h"
#include "NodeList.h"
#include "main.h"

RenderContext *renderContext = 0;
OpenGLWorkerContext *openGLWorkerContext = 0;
QSettings *settings = 0;
QSettings *outputSettings = 0;
UISettings *uiSettings = 0;
Audio *audio = 0;
OutputManager *outputManager = 0;
NodeList *nodeList = 0;
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

QObject *nodeListProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return nodeList;
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
    QObject::connect(&app, &QCoreApplication::aboutToQuit, openGLWorkerContext, &QThread::quit);

    settings = new QSettings();
    outputSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Radiance", "Radiance Output");
    uiSettings = new UISettings();
    audio = new Audio();
    outputManager = new OutputManager(outputSettings);
    timebase = new Timebase();
    nodeList = new NodeList();
    renderContext = new RenderContext();

    qmlRegisterUncreatableType<VideoNode>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterType<Model>("radiance", 1, 0, "Model");
    qmlRegisterType<EffectNode>("radiance", 1, 0, "EffectNode");
    qmlRegisterType<View>("radiance", 1, 0, "View");

    qmlRegisterType<QQuickVideoNodeRender>("radiance", 1, 0, "VideoNodeRender");
    qmlRegisterType<MidiDevice>("radiance", 1, 0, "MidiDevice");

    qmlRegisterSingletonType<UISettings>("radiance", 1, 0, "UISettings", uiSettingsProvider);
    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);
    qmlRegisterSingletonType<NodeList>("radiance", 1, 0, "NodeList", nodeListProvider);
    qmlRegisterSingletonType<RenderContext>("radiance", 1, 0, "RenderContext", renderContextProvider);

    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<LuxDevice>("radiance", 1, 0, "LuxDevice");
    qmlRegisterSingletonType<OutputManager>("radiance", 1, 0, "OutputManager", outputManagerProvider);

    QQmlApplicationEngine engine(QUrl("../resources/qml/application.qml"));
    if(engine.rootObjects().isEmpty()) {
        qFatal("Failed to load main QML application");
        return 1;
    }

    QObject *window = engine.rootObjects().first();

    return app.exec();
}
