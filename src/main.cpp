#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSize>
#include <QThread>
#include <QProcess>
#include "BaseVideoNodeTile.h"
#include "EffectNode.h"
#include "FramebufferVideoNodeRender.h"
#include "GlslDocument.h"
#include "GlslHighlighter.h"
#include "GraphicalDisplay.h"
#include "Model.h"
#include "OpenGLWorkerContext.h"
#include "FFmpegOutputNode.h"
#include "PlaceholderNode.h"
#include "Paths.h"
#include "QQuickVideoNodePreview.h"
#include "Registry.h"
#include "Timebase.h"
#include "VideoNode.h"
#include "MovieNode.h"
#include "View.h"

#ifdef USE_RTMIDI
#include "MidiController.h"
#endif

//#ifdef USE_MPV
//#include "MovieNode.h"
//#endif

#ifdef USE_LUX
#include "Lux.h"
#endif

#define IMG_FORMAT ".gif"

static int
runRadianceGui(QGuiApplication *app) {
    // Set up QML types
    qmlRegisterUncreatableType<VideoNode>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterUncreatableType<MovieNode>("radiance", 1, 0, "MovieNode", "MovieNode cannot be constructed from QML");
    qmlRegisterUncreatableType<Library>("radiance", 1, 0, "Library", "Library should be accessed through the Registry");
    qmlRegisterType<Context>("radiance", 1, 0, "Context");
    qmlRegisterType<Registry>("radiance", 1, 0, "Registry");
    qmlRegisterType<QQuickPreviewAdapter>("radiance", 1, 0, "PreviewAdapter");
    qmlRegisterType<Model>("radiance", 1, 0, "Model");
    qmlRegisterType<View>("radiance", 1, 0, "View");

    qmlRegisterType<QQuickVideoNodePreview>("radiance", 1, 0, "VideoNodePreview");
    qmlRegisterType<GlslDocument>("radiance", 1, 0, "GlslDocument");
    qmlRegisterType<GlslHighlighter>("radiance", 1, 0, "GlslHighlighter");

#ifdef USE_RTMIDI
    qmlRegisterType<MidiController>("radiance", 1, 0, "MidiController");
    bool hasMidi = true;
#else
    qInfo() << "radiance compiled without midi support";
    bool hasMidi = false;
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

    // We have to create the context here
    // (as opposed to in QML)
    // because we need to guarantee its life
    // beyond that of the Model / VideoNodes that reference it
    // and there is no way to e.g. parent it like that in QML
    Context context;
    {
        QQmlEngine engine;
        QObject::connect(&engine, &QQmlEngine::quit, app, &QGuiApplication::quit);
        engine.rootContext()->setContextProperty("defaultContext", &context);
        QQmlComponent component(&engine, QUrl(QDir::cleanPath(Paths::qml() + "/application.qml")));
        auto c = component.create();
        if(c == nullptr) {
            auto errors = component.errors();
            for (auto e = errors.begin(); e != errors.end(); e++) {
                qDebug() << *e;
            }
            qFatal("Failed to load main QML application");
            return 1;
        }
        engine.setObjectOwnership(c, QQmlEngine::JavaScriptOwnership);

        // TODO put this into a singleton
        c->setProperty("hasMidi", hasMidi);

        app->exec();
    }
    return 0;
}

static void
generateHtml(QDir outputDir, QList<VideoNode*> videoNodes) {
    QFile outputHtml(outputDir.filePath("index.html"));
    outputHtml.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream html(&outputHtml);
    html << "<!doctype html>\n";
    html << "<html><body>\n";
    html << "<style>\n";
    html << "body { color: #FFF; background-color: #111; font-family: Monospace; width: 640px; margin: auto; }\n";
    html << "h1, td, th { padding: 5px; }\n";
    html << "</style>\n";
    html << "<h1>radiance library</h1>\n";
    html << "<table><tr><th>name</th><th>0%</th><th>100%</th><th>gif</th><th>description</th><tr>\n";

    for (auto videoNode : videoNodes) {
        QString name = videoNode->property("name").toString();
        QString description = videoNode->property("description").toString();
        QString author = videoNode->property("author").toString();

        html << "<tr><td>" << name << "</td>\n";
        html << "    <td class='static'>" << "<img src='./" << name << "_0.png'>" << "</td>\n";
        html << "    <td class='static'>" << "<img src='./" << name << "_51.png'>" << "</td>\n";
        html << "    <td class='gif'>" << "<img src='./" << name << IMG_FORMAT "'>" << "</td>\n";
        html << "    <td class='desc'>" << description;
        if (!author.isNull()) {
            html << "<p>[" << author << "]</p>";
        }
        html << "</td>\n";
    }

    html << "</table>\n";
    html << "</body></html>\n";
}

static int
runRadianceCli(QGuiApplication *app, QString modelName, QString nodeFilename, QString outputDirString, QSize renderSize) {
    QDir outputDir;
    outputDir.mkpath(outputDirString);
    outputDir.cd(outputDirString);

    Registry registry;

    Context context(false);
    context.timebase()->update(Timebase::TimeSourceDiscrete, Timebase::TimeSourceEventBPM, 140.);

    Chain chain(renderSize);
    FramebufferVideoNodeRender imgRender(renderSize);

    Model model;
    model.load(&context, &registry, modelName);
    model.addChain(chain);

    FFmpegOutputNode *ffmpegNode = nullptr;
    PlaceholderNode *placeholderNode = nullptr;
    for (VideoNode *node : model.vertices()) {
        if (ffmpegNode == nullptr) {
            ffmpegNode = qobject_cast<FFmpegOutputNode *>(node);
        }
        if (placeholderNode == nullptr) {
            placeholderNode = qobject_cast<PlaceholderNode *>(node);
        }
    }
    if (ffmpegNode == nullptr) {
        qCritical() << "Unable to find FFmpegOutputNode in" << modelName;
        qCritical() << model.serialize();
        return EXIT_FAILURE;
    }
    if (placeholderNode == nullptr) {
        qCritical() << "Unable to find PlaceholderNode in" << modelName;
        qCritical() << model.serialize();
        return EXIT_FAILURE;
    }
    qInfo() << "Loaded model:" << modelName;
    qInfo() << model.serialize();

    qInfo() << "Scanning for effects in path:" << Paths::systemLibrary();
    QList<VideoNode *> renderNodes;
    if (nodeFilename.isNull()) {
        QDir libraryDir(Paths::systemLibrary());
        libraryDir.cd("effects");

        for (QString entry : libraryDir.entryList()) {
            if (entry.startsWith(".")) {
                continue;
            }
            QString entryPath = libraryDir.filePath(entry);
            VideoNode *renderNode = registry.createFromFile(&context, entryPath);
            if (!renderNode) {
                qInfo() << "Unable to open:" << entry << entryPath;
            } else {
                renderNodes << renderNode;
            }
        }

        // Generate HTML page w/ all nodes
        generateHtml(outputDir, renderNodes);
    } else {
        VideoNode *renderNode = registry.createFromFile(&context, nodeFilename);
        if (!renderNode) {
            qInfo() << "Unable to open:" << nodeFilename;
            return EXIT_FAILURE;
        }
        renderNodes << renderNode;
    }

    // Render
    for (VideoNode *renderNode : renderNodes) {
        QString name = renderNode->property("name").toString();
        qInfo() << "Rendering:" << name;

        placeholderNode->setWrappedVideoNode(renderNode);

        QString gifFilename = QString("%1" IMG_FORMAT).arg(name);
        ffmpegNode->setFFmpegArguments({outputDir.filePath(gifFilename)});
        ffmpegNode->setRecording(true);

        // Render 101 frames
        EffectNode * effectNode = qobject_cast<EffectNode *>(renderNode);
        for (int i = 0; i <= 100; i++) {
            if (effectNode != nullptr)
                effectNode->setIntensity(i / 50.);

            context.timebase()->update(Timebase::TimeSourceDiscrete, Timebase::TimeSourceEventBeat, i / 12.5);

            auto modelCopy = model.createCopyForRendering();
            auto rendering = modelCopy.render(chain);

            auto outputTextureId = rendering.value(*ffmpegNode, 0);
            if (outputTextureId != 0) {
                if (i == 0 || i == 51) {
                    QImage img = imgRender.render(outputTextureId);
                    QString filename = outputDir.filePath(QString("%1_%2.png").arg(name, QString::number(i)));
                    img.save(filename);
                }
            }
            ffmpegNode->recordFrame();
        }

        // Reset state
        ffmpegNode->setRecording(false);
    }

    return 0;
}

int
main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.video");
    QCoreApplication::setApplicationName("Radiance");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
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

    QCommandLineParser parser;
    parser.addHelpOption();

    const QCommandLineOption outputDirOption(QStringList() << "o" << "output", "Output Directory", "path");
    parser.addOption(outputDirOption);
    const QCommandLineOption modelOption(QStringList() << "m" << "model", "Load this Model file", "json");
    parser.addOption(modelOption);
    const QCommandLineOption nodeFilenameOption(QStringList() << "n" << "node", "Render this file", "file");
    parser.addOption(nodeFilenameOption);
    const QCommandLineOption renderAllOption(QStringList() << "a" << "all", "Render all effects in the library");
    parser.addOption(renderAllOption);
    const QCommandLineOption sizeOption(QStringList() << "s" << "size", "Render using this size [128x128]", "wxh");
    parser.addOption(sizeOption);

    parser.process(app);

    QString outputDirString("render_output");
    if (parser.isSet(outputDirOption)) {
        outputDirString = parser.value(outputDirOption);
    }

    QString modelName("cli");
    if (parser.isSet(modelOption)) {
        modelName = parser.value(modelOption);
    }

    QSize renderSize(128, 128);
    if (parser.isSet(sizeOption)) {
        QString rawSize = parser.value(sizeOption);
        int x = rawSize.indexOf('x');
        if (x >= 0) {
            renderSize.rwidth() = rawSize.mid(x + 1).toInt();
            renderSize.rheight() = rawSize.left(x).toInt();
        }
        //TODO: handle failure
    }

    if (parser.isSet(nodeFilenameOption) || parser.isSet(renderAllOption)) {
        return runRadianceCli(&app, modelName, parser.value(nodeFilenameOption), outputDirString, renderSize);
    } else {
        return runRadianceGui(&app);
    }
}
