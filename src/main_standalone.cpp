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
#include "NodeList.h"
#include "main.h"

QSharedPointer<RenderContext> renderContext;
QSharedPointer<OpenGLWorkerContext> openGLWorkerContext;
QSharedPointer<QSettings> settings;
QSharedPointer<Audio> audio;
QSharedPointer<NodeList> nodeList;
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
    nodeList = QSharedPointer<NodeList>(new NodeList());
    renderContext = QSharedPointer<RenderContext>(new RenderContext());

    EffectNode * testEffect = new EffectNode();
    testEffect->setName("test");
    testEffect->setIntensity(0.7);

    Model * model = new Model();
    model->addVideoNode(testEffect);

    FramebufferVideoNodeRender *imgRender = new FramebufferVideoNodeRender();
    imgRender->setChain(0);

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
    html << "<style>td.static { background-color: #000; }; td.gif { }</style>\n";
    html << "<table><tr><th>name</th><th>comment</th><th>static</th><th>gif</th><tr>\n";

    for (auto effectName : nodeList->effectNames()) {
        EffectNode effect;
        effect.setName(effectName);
        effect.setIntensity(1.0);
        model->addVideoNode(&effect);
        model->addEdge(testEffect, &effect, 0);
        imgRender->setVideoNode(&effect);
        outputDir.mkdir(effectName);
        for (int i = 1; i <= 100; i++) {
            timebase->update(Timebase::TimeSourceDiscrete, Timebase::TimeSourceEventBeat, i / 25.0);
            renderContext->periodic();

            renderContext->render(model, 0);
            QImage img = imgRender->render();
            QString filename = QString("%1/%2/%3.png").arg(outputDir.path(), effectName, QString::number(i));
            img.save(filename);
        }
        model->removeEdge(testEffect, &effect, 0);
        model->removeVideoNode(&effect);

        QProcess ffmpeg;
        ffmpeg.start("ffmpeg",
            QStringList()
                << "-y" << "-i"
                << QString("%1/%2/%d.png").arg(outputDir.path(), effectName)
                << QString("%1/%2.gif").arg(outputDir.path(), effectName));
        ffmpeg.waitForFinished();
        qInfo() << "Rendered" << effectName << ffmpeg.exitCode() << ffmpeg.exitStatus();

        html << "<tr><td>" << effectName << "</td>\n";
        html << "    <td>" << "" << "</td>\n";
        html << "    <td class='static'>" << "<img src='./" << effectName << "/1.png'>" << "</td>\n";
        html << "    <td class='gif'>" << "<img src='./" << effectName << ".gif'>" << "</td>\n";
        html.flush();
    }

    html << "</table>\n";
    html << "</body></html>\n";
    html.flush();

    app.quit();
    return 0;
}
