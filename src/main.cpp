#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "EffectNode.h"
#include "GraphicalDisplay.h"
#include "ImageNode.h"
#include "Lux.h"
#include "Model.h"
#include "NodeRegistry.h"
#include "Output.h"
#include "QQuickVideoNodePreview.h"
#include "QQuickOutputItem.h"
#include "QQuickOutputWindow.h"
#include "OutputImageSequence.h"
#include "VideoNode.h"
#include "View.h"
#include "main.h"

#if RTMIDI_FOUND == TRUE
#include "Midi.h"
#endif 

#if MPV_FOUND == TRUE
#include "MovieNode.h"
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
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);

    QThread::currentThread()->setObjectName("mainThread");

    openGLWorkerContext = new OpenGLWorkerContext();
    openGLWorkerContext->setParent(&app);

    settings = QSharedPointer<QSettings>(new QSettings());
    outputSettings = QSharedPointer<QSettings>(new QSettings(QSettings::IniFormat, QSettings::UserScope, "Radiance", "Radiance Output"));
    timebase = QSharedPointer<Timebase>(new Timebase());
    audio = QSharedPointer<Audio>(new Audio());
    nodeRegistry = QSharedPointer<NodeRegistry>(new NodeRegistry());

    qmlRegisterUncreatableType<VideoNode>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterType<Context>("radiance", 1, 0, "Context");
    qmlRegisterType<Model>("radiance", 1, 0, "Model");
    qmlRegisterType<EffectNode>("radiance", 1, 0, "EffectNode");
    qmlRegisterType<ImageNode>("radiance", 1, 0, "ImageNode");
#if MPV_FOUND == TRUE
    qmlRegisterType<MovieNode>("radiance", 1, 0, "MovieNode");
#else
    qInfo() << "radiance compiled without mpv support";
#endif
    qmlRegisterType<View>("radiance", 1, 0, "View");

    qmlRegisterType<QQuickVideoNodePreview>("radiance", 1, 0, "VideoNodePreview");
    qmlRegisterType<QQuickOutputItem>("radiance", 1, 0, "OutputItem");
    qmlRegisterType<QQuickOutputWindow>("radiance", 1, 0, "OutputWindow");
    qmlRegisterType<OutputImageSequence>("radiance", 1, 0, "OutputImageSequence");
#if RTMIDI_FOUND == TRUE
    qmlRegisterType<MidiDevice>("radiance", 1, 0, "MidiDevice");
#else
    qInfo() << "radiance compiled without midi support";
#endif
    qmlRegisterType<GraphicalDisplay>("radiance", 1, 0, "GraphicalDisplay");

    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);
    qmlRegisterSingletonType<NodeRegistry>("radiance", 1, 0, "NodeRegistry", nodeRegistryProvider);

    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<LuxDevice>("radiance", 1, 0, "LuxDevice");

    QQmlApplicationEngine engine(QUrl("../resources/qml/application.qml"));
    if(engine.rootObjects().isEmpty()) {
        qFatal("Failed to load main QML application");
        return 1;
    }

    return app.exec();
}
