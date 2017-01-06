#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "EffectUI.h"
#include "CrossFaderUI.h"
#include "RenderContext.h"
#include "Output.h"
#include "Lux.h"
#include "Audio.h"
#include "main.h"

RenderContext *renderContext = 0;
QSettings *settings = 0;
UISettings *uiSettings = 0;
Audio *audio = 0;

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

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");

    //qRegisterMetaType<Effect*>("Effect*");

    settings = new QSettings();
    uiSettings = new UISettings();
    audio = new Audio();

    // Render thread
    QThread renderThread;
    renderThread.setObjectName("RenderThread");
    renderContext = new RenderContext();
    renderContext->moveToThread(&renderThread);
    QObject::connect(&renderThread, &QThread::started, renderContext, &RenderContext::start);
    //QObject::connect(&renderThread, &QThread::finished, renderContext, &RenderContext::finish);
    renderThread.start();

    qmlRegisterUncreatableType<VideoNodeUI>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterType<EffectUI>("radiance", 1, 0, "Effect");
    qmlRegisterType<CrossFaderUI>("radiance", 1, 0, "CrossFader");

    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<OutputManager>("radiance", 1, 0, "OutputManager");

    qmlRegisterSingletonType<UISettings>("radiance", 1, 0, "UISettings", uiSettingsProvider);
    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);

    QQmlApplicationEngine engine(QUrl("../resources/qml/application.qml"));

    QObject *window = engine.rootObjects().first();
    QObject::connect(window, SIGNAL(frameSwapped()), renderContext, SLOT(render()));

    return app.exec();
}
