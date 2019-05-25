#include "ScreenOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

ScreenOutputNode::ScreenOutputNode(Context *context, QSize chainSize)
    : OutputNode(context, chainSize)
{
    m_outputWindow = QSharedPointer<OutputWindow>(new OutputWindow(qSharedPointerCast<OutputNode>(sharedFromThis())));
    connect(m_outputWindow.data(), &OutputWindow::screenNameChanged, this, &ScreenOutputNode::screenNameChanged);
    connect(m_outputWindow.data(), &OutputWindow::screenSizeChanged, this, &ScreenOutputNode::onScreenSizeChanged);
    connect(m_outputWindow.data(), &OutputWindow::shownChanged, this, &ScreenOutputNode::shownChanged);
    connect(m_outputWindow.data(), &OutputWindow::foundChanged, this, &ScreenOutputNode::foundChanged);

    QSize native = m_outputWindow->screenSize();
    resize(native);
    onScreenSizeChanged(native);

    reload();
    connect(&m_reloader, &QTimer::timeout, this, &ScreenOutputNode::reload);
    m_reloader.setInterval(1000); // Reload screens every 1000 ms
    m_reloader.start();
}

void ScreenOutputNode::setShown(bool shown) {
    m_outputWindow->setShown(shown);
}

bool ScreenOutputNode::shown() {
    return m_outputWindow->shown();
}

bool ScreenOutputNode::found() {
    return m_outputWindow->found();
}

void ScreenOutputNode::reload() {
    auto screens = QGuiApplication::screens();

    if (screens != m_screens) {
        m_screens = screens;

        m_screenNameStrings.clear();

        foreach(QScreen *screen, m_screens) {
            m_screenNameStrings << screen->name();
        }

        emit availableScreensChanged(m_screenNameStrings);
    }
}

QStringList ScreenOutputNode::availableScreens() {
    return m_screenNameStrings;
}

QString ScreenOutputNode::screenName() {
    return m_outputWindow->screenName();
}

void ScreenOutputNode::setScreenName(QString screenName) {
    m_outputWindow->setScreenName(screenName);
}

QSize ScreenOutputNode::resolution() {
    return chain()->size();
}

void ScreenOutputNode::setResolution(QSize resolution) {
    if (resolution != chain()->size()) {
        resize(resolution);
        emit resolutionChanged(resolution);
    }
}

QVariantList ScreenOutputNode::suggestedResolutions() {
    QVariantList result;
    for (auto res : m_suggestedResolutions) {
        result << res;
    }
    return result;
}

void ScreenOutputNode::onScreenSizeChanged(QSize size) {
    QList<QSize> newSuggestedResolutions;
    newSuggestedResolutions << size;
    auto aspect = (float)size.width() / size.height();
    auto allowSquish = 1.1f;
    for (auto res : commonResolutions) {
        auto testAspect = (float)res.width() / res.height();
        if (testAspect / aspect > 1.f / allowSquish
         && testAspect / aspect < allowSquish) {
            newSuggestedResolutions << res;
        }
    }
    if (newSuggestedResolutions != m_suggestedResolutions) {
        m_suggestedResolutions = newSuggestedResolutions;
        emit suggestedResolutionsChanged(suggestedResolutions());
    }
}

QString ScreenOutputNode::typeName() {
    return "ScreenOutputNode";
}

VideoNodeSP *ScreenOutputNode::deserialize(Context *context, QJsonObject obj) {
    auto e = new ScreenOutputNode(context, QSize(640,480));
    return new VideoNodeSP(e);
}

bool ScreenOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNodeSP *ScreenOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> ScreenOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("ScreenOutput", "ScreenOutputInstantiator.qml");
    return m;
}

QList<QSize> ScreenOutputNode::commonResolutions = QList<QSize>({
    QSize(320, 200),
    QSize(320, 240),
    QSize(352, 288),
    QSize(384, 288),
    QSize(480, 320),
    QSize(640, 480),
    QSize(800, 480),
    QSize(854, 480),
    QSize(768, 576),
    QSize(800, 600),
    QSize(1024, 576),
    QSize(1024, 600),
    QSize(1024, 768),
    QSize(1152, 768),
    QSize(1280, 720),
    QSize(1280, 768),
    QSize(1152, 864),
    QSize(1280, 800),
    QSize(1366, 768),
    QSize(1280, 854),
    QSize(1280, 960),
    QSize(1440, 900),
    QSize(1280, 1024),
    QSize(1440, 960),
    QSize(1600, 900),
    QSize(1400, 1050),
    QSize(1440, 1080),
    QSize(1680, 1050),
    QSize(1680, 1050),
    QSize(1600, 1200),
    QSize(1920, 1080),
    QSize(2048, 1080),
    QSize(1920, 1200),
    QSize(2560, 1080),
    QSize(2048, 1536),
    QSize(2560, 1440),
    QSize(2560, 1600),
    QSize(3440, 1440),
    QSize(2560, 2048),
    QSize(3840, 2160),
    QSize(4096, 2160),
});
