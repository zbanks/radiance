#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "GraphicalDisplay.h"
#include "Model.h"
#include "QQuickVideoNodePreview.h"
#include "VideoNode.h"
#include "BaseVideoNodeTile.h"
#include "View.h"
#include "Paths.h"
#include "Registry.h"

#ifdef USE_RTMIDI
#include "MidiController.h"
#endif

//#ifdef USE_MPV
//#include "MovieNode.h"
//#endif

#ifdef USE_LUX
#include "Lux.h"
#endif

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.video");
    QCoreApplication::setApplicationName("Radiance");
    // Use OpenGL 3.2 core profile
    // (otherwise MacOS falls back to 2.1)
    {
        auto format = QSurfaceFormat::defaultFormat();
        format.setVersion(3, 2);
        format.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    }
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);

    Paths::initialize();

#ifndef USE_MPV
    qInfo() << "radiance compiled without mpv support";
#endif

    QThread::currentThread()->setObjectName("mainThread");

    qmlRegisterUncreatableType<VideoNode>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterUncreatableType<Library>("radiance", 1, 0, "Library", "Library should be accessed through the Registry");
    qmlRegisterType<Context>("radiance", 1, 0, "Context");
    qmlRegisterType<Registry>("radiance", 1, 0, "Registry");
    qmlRegisterType<QQuickPreviewAdapter>("radiance", 1, 0, "PreviewAdapter");
    qmlRegisterType<Model>("radiance", 1, 0, "Model");
    qmlRegisterType<View>("radiance", 1, 0, "View");

    qmlRegisterType<QQuickVideoNodePreview>("radiance", 1, 0, "VideoNodePreview");

    bool hasMidi = false;
#ifdef USE_RTMIDI
    qmlRegisterType<MidiController>("radiance", 1, 0, "MidiController");
    hasMidi = true;
#else
    qInfo() << "radiance compiled without midi support";
#endif
    qmlRegisterType<GraphicalDisplay>("radiance", 1, 0, "GraphicalDisplay");

#ifdef USE_LUX
    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<LuxDevice>("radiance", 1, 0, "LuxDevice");
#else
    qInfo() << "radiance compiled without lux support";
#endif

    qmlRegisterType<BaseVideoNodeTile>("radiance", 1, 0, "BaseVideoNodeTile");

    qmlRegisterUncreatableType<Controls>("radiance", 1, 0, "Controls", "Controls cannot be instantiated");
    qRegisterMetaType<Controls::Control>("Controls::Control");

    QQmlApplicationEngine engine(QUrl(Paths::qml() + "/application.qml"));

    if(engine.rootObjects().isEmpty()) {
        qFatal("Failed to load main QML application");
        return 1;
    }

    // TODO put these into a singleton
    engine.rootObjects().last()->setProperty("hasMidi", hasMidi);

    app.exec();
}
