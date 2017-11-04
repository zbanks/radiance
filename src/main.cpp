#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "EffectNode.h"
#include "GraphicalDisplay.h"
#include "ImageNode.h"
#include "Model.h"
#include "NodeRegistry.h"
#include "Output.h"
#include "QQuickVideoNodePreview.h"
#include "QQuickOutputItem.h"
#include "QQuickOutputWindow.h"
#include "OutputImageSequence.h"
#include "VideoNode.h"
#include "BaseVideoNodeTile.h"
#include "View.h"
#include "Paths.h"
#include "main.h"

#ifdef USE_RTMIDI
#include "MidiController.h"
#endif

#ifdef USE_MPV
#include "MovieNode.h"
#endif

#ifdef USE_LUX
#include "Lux.h"
#endif

OpenGLWorkerContext *openGLWorkerContext;
QSharedPointer<QSettings> settings;
QSharedPointer<QSettings> outputSettings;
QSharedPointer<Audio> audio;
QSharedPointer<NodeRegistry> nodeRegistry;
QSharedPointer<Timebase> timebase;

QObject *audioProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    QQmlEngine::setObjectOwnership(audio.data(), QQmlEngine::CppOwnership);
    return audio.data();
}

QObject *nodeRegistryProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    QQmlEngine::setObjectOwnership(nodeRegistry.data(), QQmlEngine::CppOwnership);
    return nodeRegistry.data();
}

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

#ifdef DEBUG_RESOURCES
    Paths::initialize(true);
#else
    Paths::initialize();
#endif

    QThread::currentThread()->setObjectName("mainThread");

    openGLWorkerContext = new OpenGLWorkerContext();
    openGLWorkerContext->setParent(&app);

    settings = QSharedPointer<QSettings>(new QSettings());
    outputSettings = QSharedPointer<QSettings>(new QSettings(QSettings::IniFormat, QSettings::UserScope, "Radiance", "Radiance Output"));
    timebase = QSharedPointer<Timebase>(new Timebase());
    audio = QSharedPointer<Audio>(new Audio());

    nodeRegistry = QSharedPointer<NodeRegistry>(new NodeRegistry());
    qmlRegisterType<EffectNode>("radiance", 1, 0, "EffectNode");
    qmlRegisterType<ImageNode>("radiance", 1, 0, "ImageNode");
#ifdef USE_MPV
    qmlRegisterType<MovieNode>("radiance", 1, 0, "MovieNode");
#else
    qInfo() << "radiance compiled without mpv support";
#endif
    nodeRegistry->reload();

    qmlRegisterUncreatableType<VideoNode>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterType<Context>("radiance", 1, 0, "Context");
    qmlRegisterType<Model>("radiance", 1, 0, "Model");
    qmlRegisterType<View>("radiance", 1, 0, "View");

    qmlRegisterType<QQuickVideoNodePreview>("radiance", 1, 0, "VideoNodePreview");
    qmlRegisterType<QQuickOutputItem>("radiance", 1, 0, "OutputItem");
    qmlRegisterType<QQuickOutputWindow>("radiance", 1, 0, "OutputWindow");
    qmlRegisterType<OutputImageSequence>("radiance", 1, 0, "OutputImageSequence");

    bool hasMidi = false;
#ifdef USE_RTMIDI
    qmlRegisterType<MidiController>("radiance", 1, 0, "MidiController");
    hasMidi = true;
#else
    qInfo() << "radiance compiled without midi support";
#endif
    qmlRegisterType<GraphicalDisplay>("radiance", 1, 0, "GraphicalDisplay");

    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);
    qmlRegisterSingletonType<NodeRegistry>("radiance", 1, 0, "NodeRegistry", nodeRegistryProvider);

#ifdef USE_LUX
    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<LuxDevice>("radiance", 1, 0, "LuxDevice");
#else
    qInfo() << "radiance compiled without lux support";
#endif

    qmlRegisterType<BaseVideoNodeTile>("radiance", 1, 0, "BaseVideoNodeTile");

    qmlRegisterUncreatableType<Controls>("radiance", 1, 0, "Controls", "Controls cannot be instantiated");
    qRegisterMetaType<Controls::Control>("Controls::Control");

    QQmlApplicationEngine engine(QUrl(Paths::qml() + QString("application.qml")));

    if(engine.rootObjects().isEmpty()) {
        qFatal("Failed to load main QML application");
        return 1;
    }

    // TODO put these into a singleton
    engine.rootObjects().last()->setProperty("hasMidi", hasMidi);

    return app.exec();
}
