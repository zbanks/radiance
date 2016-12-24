#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include "Effect.h"
#include "RenderThread.h"
#include "Output.h"
#include "Lux.h"
#include "Audio.h"
#include "main.h"

RenderThread *renderThread = 0;
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

    settings = new QSettings();
    uiSettings = new UISettings();
    audio = new Audio();
    renderThread = new RenderThread();

    qmlRegisterType<Effect>("radiance", 1, 0, "Effect");
    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<OutputManager>("radiance", 1, 0, "OutputManager");

    qmlRegisterSingletonType<UISettings>("radiance", 1, 0, "UISettings", uiSettingsProvider);
    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);

    QQmlApplicationEngine engine(QUrl("qrc:///qml/application.qml"));

    return app.exec();
}
