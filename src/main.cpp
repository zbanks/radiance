#include <QGuiApplication>
#include <QQuickView>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQuickView view;
    view.setSource(QUrl("qrc:///qml/application.qml"));
    view.show();

    return app.exec();
}
