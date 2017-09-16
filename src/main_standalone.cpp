#include <QGuiApplication>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QThread>
#include <QDir>
#include <QProcess>
#include <QFile>
#include "VideoNode.h"
#include "FramebufferVideoNodeRender.h"
#include "Model.h"
#include "View.h"
#include "EffectNode.h"
#include "ImageNode.h"
#include "NodeRegistry.h"
#include "main.h"

#define IMG_FORMAT ".gif"

QSharedPointer<OpenGLWorkerContext> openGLWorkerContext;
QSharedPointer<QSettings> settings;
QSharedPointer<Audio> audio;
QSharedPointer<NodeRegistry> nodeRegistry;
QSharedPointer<Timebase> timebase;

void generateHtml(QDir outputDir, QList<QString> nodeNames) {
    QFile outputHtml(outputDir.filePath("index.html"));
    outputHtml.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream html(&outputHtml);
    html << "<!doctype html>\n";
    html << "<html><body>\n";
    html << "<style>td.static, td.gif { background-color: #FFF; }</style>\n";
    html << "<table><tr><th>name</th><th>comment</th><th>0</th><th>static</th><th>gif</th><tr>\n";

    for (auto name : nodeNames) {
        html << "<tr><td>" << name << "</td>\n";
        html << "    <td>" << "" << "</td>\n";
        html << "    <td class='static'>" << "<img src='./" << name << "/0.png'>" << "</td>\n";
        html << "    <td class='static'>" << "<img src='./" << name << "/51.png'>" << "</td>\n";
        html << "    <td class='gif'>" << "<img src='./" << name << IMG_FORMAT "'>" << "</td>\n";
    }

    html << "</table>\n";
    html << "</body></html>\n";
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv); // TODO: how to avoid this?

    QCommandLineParser parser;
    parser.addHelpOption();

    const QCommandLineOption outputDirOption(QStringList() << "o" << "output", "Output Directory", "path");
    parser.addOption(outputDirOption);
    const QCommandLineOption onlyNodeOption(QStringList() << "n" << "only-node", "Only render this node type", "node");
    parser.addOption(onlyNodeOption);
    const QCommandLineOption backgroundNodeOption(QStringList() << "b" << "background-node", "Use this node string for the background", "node");
    parser.addOption(backgroundNodeOption);
    const QCommandLineOption sizeOption(QStringList() << "s" << "size", "Render using this size [128x128]", "wxh");
    parser.addOption(sizeOption);
    //TODO:
    //const QCommandLineOption treeOption(QStringList() << "t" << "tree", "Render this node tree", "nodes");
    //parser.addOption(treeOption);

    parser.process(app);

    QThread::currentThread()->setObjectName("mainThread");

    openGLWorkerContext = QSharedPointer<OpenGLWorkerContext>(new OpenGLWorkerContext(false));
    openGLWorkerContext->setObjectName("openGLWorkerContext");
    openGLWorkerContext->thread()->start();

    settings = QSharedPointer<QSettings>(new QSettings());
    audio = QSharedPointer<Audio>(new Audio());
    timebase = QSharedPointer<Timebase>(new Timebase());
    nodeRegistry = QSharedPointer<NodeRegistry>(new NodeRegistry());

    timebase->update(Timebase::TimeSourceDiscrete, Timebase::TimeSourceEventBPM, 140.);

    // Set up output directory
    QString outputDirString("render_output");
    if (parser.isSet(outputDirOption)) {
        outputDirString = parser.value(outputDirOption);
    }

    QDir outputDir;
    outputDir.mkpath("render_output");
    outputDir.cd("render_output");
    //outputDir.removeRecursively();
    //qInfo() << "Wiped" << outputDir.absolutePath();

    // Generate HTML page with previews
    QList<QString> nodeNames;
    for (auto nodeType : nodeRegistry->nodeTypes()) {
        if (nodeType.nInputs > 1)
            continue;
        nodeNames.append(nodeType.name);
    }
    generateHtml(outputDir, nodeNames);

    // Set up output chain & FBO renderer
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
    QList<QSharedPointer<Chain>> chains;
    QSharedPointer<Chain> chain(new Chain(renderSize));
    chains.append(chain);

    FramebufferVideoNodeRender imgRender(renderSize);

    // Set up Model & common effects
    Model model;
    model.setChains(chains);
    VideoNode *onblackEffect = model.createVideoNode("onblack:1.0");

    VideoNode *baseEffect = nullptr;
    if (parser.isSet(backgroundNodeOption)) {
        QString baseEffectName = parser.value(backgroundNodeOption);
        baseEffect = model.createVideoNode(baseEffectName);
        qInfo() << "Rendering with background:" << baseEffectName;
    } else {
        baseEffect = model.createVideoNode("test:0.7");
        //baseEffect = model.createVideoNode("nyancat.gif");
    }
    if (!baseEffect) {
        qInfo() << "Unable to set up background effect";
        return EXIT_FAILURE;
    }

    // Render node(s)
    if (parser.isSet(onlyNodeOption)) {
        nodeNames.clear();
        nodeNames.append(parser.value(onlyNodeOption));
    }

    for (auto nodeName : nodeNames) {
        VideoNode *effect = model.createVideoNode(nodeName);
        if (!effect)
            continue;
        EffectNode * effectNode = qobject_cast<EffectNode *>(effect);

        model.addEdge(baseEffect, effect, 0);
        model.addEdge(effect, onblackEffect, 0);
        model.flush();
        qInfo() << model.serialize();

        outputDir.mkdir(nodeName);
        for (int i = 0; i <= 100; i++) {
            if (effectNode)
                effectNode->setIntensity(i / 50.);
            timebase->update(Timebase::TimeSourceDiscrete, Timebase::TimeSourceEventBeat, i / 25.0);

            auto modelCopy = model.createCopyForRendering();
            auto rendering = modelCopy.render(chain);
            model.copyBackRenderStates(chain, &modelCopy);

            auto outputTextureId = rendering.value(onblackEffect->id(), 0);
            if (outputTextureId != 0) {
                QImage img = imgRender.render(outputTextureId);
                QString filename = QString("%1/%2/%3.png").arg(outputDir.path(), nodeName, QString::number(i));
                img.save(filename);
            }
        }

        // ffmpeg: Easiest way to convert images into a GIF other format
        QProcess ffmpeg;
        ffmpeg.start("ffmpeg",
            QStringList()
                << "-y" << "-i"
                << QString("%1/%2/%d.png").arg(outputDir.path(), nodeName)
                << QString("%1/%2" IMG_FORMAT).arg(outputDir.path(), nodeName));
        ffmpeg.waitForFinished();
        qInfo() << "Rendered" << nodeName << ffmpeg.exitCode() << ffmpeg.exitStatus();

        model.removeVideoNode(effect); // implicitly deletes edges
        model.flush();
    }

    qInfo() << "Done.";

    app.quit();
    return 0;
}
