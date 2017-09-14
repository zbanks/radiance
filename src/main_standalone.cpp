#include <QGuiApplication>
#include <QCoreApplication>
#include <QThread>
#include <QDir>
#include <QProcess>
#include <QFile>
#include "VideoNode.h"
#include "FramebufferVideoNodeRender.h"
#include "Model.h"
#include "View.h"
#include "EffectNode.h"
#include "NodeRegistry.h"
#include "main.h"

QSharedPointer<OpenGLWorkerContext> openGLWorkerContext;
QSharedPointer<QSettings> settings;
QSharedPointer<Audio> audio;
QSharedPointer<NodeRegistry> nodeRegistry;
QSharedPointer<Timebase> timebase;

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);

    QThread::currentThread()->setObjectName("mainThread");

    openGLWorkerContext = QSharedPointer<OpenGLWorkerContext>(new OpenGLWorkerContext(false));
    openGLWorkerContext->setObjectName("openGLWorkerContext");
    openGLWorkerContext->thread()->start();

    settings = QSharedPointer<QSettings>(new QSettings());
    audio = QSharedPointer<Audio>(new Audio());
    timebase = QSharedPointer<Timebase>(new Timebase());
    nodeRegistry = QSharedPointer<NodeRegistry>(new NodeRegistry());

    QSize renderSize(300, 300);
    QList<QSharedPointer<Chain>> chains;
    chains.append(QSharedPointer<Chain>(new Chain(renderSize)));

    EffectNode * testEffect = new EffectNode();
    testEffect->setName("test");
    testEffect->setIntensity(0.7);

    Model * model = new Model();
    model->addVideoNode(testEffect);
    model->setChains(chains);

    FramebufferVideoNodeRender *imgRender = new FramebufferVideoNodeRender(renderSize);

    timebase->update(Timebase::TimeSourceDiscrete, Timebase::TimeSourceEventBPM, 140.);

    QDir outputDir;
    outputDir.mkpath("render_output");
    outputDir.cd("render_output");
    //outputDir.removeRecursively();
    //qInfo() << "Wiped" << outputDir.absolutePath();

    QFile outputHtml(outputDir.filePath("output.html"));
    outputHtml.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream html(&outputHtml);
    html << "<!doctype html>\n";
    html << "<html><body>\n";
    html << "<style>td.static, td.gif { background-color: #FFF; }</style>\n";
    html << "<table><tr><th>name</th><th>comment</th><th>static</th><th>gif</th><tr>\n";

    for (auto nodeType : nodeRegistry->nodeTypes()) {
        QSharedPointer<VideoNode> effect = nodeRegistry->createNode(nodeType.name);
        if (!effect)
            continue;
        EffectNode * effectNode = qobject_cast<EffectNode *>(effect.data());

        model->addVideoNode(effect.data());
        model->addEdge(testEffect, effect.data(), 0);
        model->flush();

        outputDir.mkdir(nodeType.name);
        for (int i = 1; i <= 100; i++) {
            if (effectNode)
                effectNode->setIntensity(i / 50.);
            timebase->update(Timebase::TimeSourceDiscrete, Timebase::TimeSourceEventBeat, i / 25.0);

            //auto rendering = model->render(chains.at(0));
            //auto outputTextureId = rendering.value(effect->id(), 0);
            GLuint outputTextureId = 0; // XXX FIXME
            if (outputTextureId != 0) {
                QImage img = imgRender->render(outputTextureId);
                QString filename = QString("%1/%2/%3.png").arg(outputDir.path(), nodeType.name, QString::number(i));
                img.scaled(QSize(80, 80)).save(filename);
            }
        }
        model->removeEdge(testEffect, effect.data(), 0);
        model->removeVideoNode(effect.data());
        model->flush();

        QProcess ffmpeg;
        ffmpeg.start("ffmpeg",
            QStringList()
                << "-y" << "-i"
                << QString("%1/%2/%d.png").arg(outputDir.path(), nodeType.name)
                << QString("%1/%2.webp").arg(outputDir.path(), nodeType.name));
        ffmpeg.waitForFinished();
        qInfo() << "Rendered" << nodeType.name << ffmpeg.exitCode() << ffmpeg.exitStatus();

        html << "<tr><td>" << nodeType.name << "</td>\n";
        html << "    <td>" << "" << "</td>\n";
        html << "    <td class='static'>" << "<img src='./" << nodeType.name << "/51.png'>" << "</td>\n";
        html << "    <td class='gif'>" << "<img src='./" << nodeType.name << ".webp'>" << "</td>\n";
        html.flush();
    }

    html << "</table>\n";
    html << "</body></html>\n";
    html.flush();

    app.quit();
    return 0;
}
