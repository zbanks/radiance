#include <QGuiApplication>
#include <QQuickView>
#include "effect.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    qmlRegisterType<Effect>("radiance", 1, 0, "Effect");

    QQuickView view;
    view.setSource(QUrl("qrc:///qml/application.qml"));
    view.show();

    return app.exec();
}
