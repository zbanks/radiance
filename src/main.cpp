#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include "Effect.h"
#include "RenderThread.h"
#include "main.h"

RenderThread *renderThread = 0;
QSettings *settings = 0;
UISettings *uiSettings = 0;

QObject *uiSettingsProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return uiSettings;
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");

    settings = new QSettings();
    uiSettings = new UISettings();
    renderThread = new RenderThread();
    renderThread->start();

    qmlRegisterType<Effect>("radiance", 1, 0, "Effect");

    qmlRegisterSingletonType<UISettings>("radiance", 1, 0, "UISettings", uiSettingsProvider);

    QQmlApplicationEngine engine(QUrl("qrc:///qml/application.qml"));

    return app.exec();
}
