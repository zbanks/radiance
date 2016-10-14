#include <QGuiApplication>
#include <QQuickView>
#include <QThread>
#include "effect.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    Effect::renderThread = new QThread();
    Effect::renderThread->start();

    qmlRegisterType<Effect>("radiance", 1, 0, "Effect");

    QQuickView view;
    view.setSource(QUrl("qrc:///qml/application.qml"));
    view.show();

    // TODO teardown renderThread

    return app.exec();
}
