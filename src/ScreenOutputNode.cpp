#include "ScreenOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

ScreenOutputNode::ScreenOutputNode(Context *context, QSize chainSize)
    : OutputNode(new ScreenOutputNodePrivate(context, chainSize))
{
    d()->m_outputWindow = QSharedPointer<OutputWindow>(new OutputWindow(this));
    connect(d()->m_outputWindow.data(), &OutputWindow::screenNameChanged, this, &ScreenOutputNode::screenNameChanged);
    connect(d()->m_outputWindow.data(), &OutputWindow::screenSizeChanged, this, &ScreenOutputNode::onScreenSizeChanged);
    connect(d()->m_outputWindow.data(), &OutputWindow::shownChanged, this, &ScreenOutputNode::shownChanged);
    connect(d()->m_outputWindow.data(), &OutputWindow::foundChanged, this, &ScreenOutputNode::foundChanged);

    QSize native = d()->m_outputWindow->screenSize();
    resize(native);
    onScreenSizeChanged(native);

    reload();
    connect(&d()->m_reloader, &QTimer::timeout, this, &ScreenOutputNode::reload);
    d()->m_reloader.setInterval(1000); // Reload screens every 1000 ms
    d()->m_reloader.start();
}

ScreenOutputNode::ScreenOutputNode(const ScreenOutputNode &other)
    : OutputNode(other)
{
}

ScreenOutputNode *ScreenOutputNode::clone() const {
    return new ScreenOutputNode(*this);
}

QSharedPointer<ScreenOutputNodePrivate> ScreenOutputNode::d() {
    return d_ptr.staticCast<ScreenOutputNodePrivate>();
}

void ScreenOutputNode::setShown(bool shown) {
    d()->m_outputWindow->setShown(shown);
}

bool ScreenOutputNode::shown() {
    return d()->m_outputWindow->shown();
}

bool ScreenOutputNode::found() {
    return d()->m_outputWindow->found();
}

void ScreenOutputNode::reload() {
    auto screens = QGuiApplication::screens();

    if (screens != d()->m_screens) {
        d()->m_screens = screens;

        d()->m_screenNameStrings.clear();

        foreach(QScreen *screen, d()->m_screens) {
            d()->m_screenNameStrings << screen->name();
        }

        emit availableScreensChanged(d()->m_screenNameStrings);
    }
}

QStringList ScreenOutputNode::availableScreens() {
    return d()->m_screenNameStrings;
}

QString ScreenOutputNode::screenName() {
    return d()->m_outputWindow->screenName();
}

void ScreenOutputNode::setScreenName(QString screenName) {
    d()->m_outputWindow->setScreenName(screenName);
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
    for (auto res : d()->m_suggestedResolutions) {
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
    if (newSuggestedResolutions != d()->m_suggestedResolutions) {
        d()->m_suggestedResolutions = newSuggestedResolutions;
        emit suggestedResolutionsChanged(suggestedResolutions());
    }
}

QString ScreenOutputNode::typeName() {
    return "ScreenOutputNode";
}

VideoNode *ScreenOutputNode::deserialize(Context *context, QJsonObject obj) {
    ScreenOutputNode *e = new ScreenOutputNode(context, QSize(640,480));
    return e;
}

bool ScreenOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *ScreenOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> ScreenOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("ScreenOutput", "ScreenOutputInstantiator.qml");
    return m;
}

ScreenOutputNodePrivate::ScreenOutputNodePrivate(Context *context, QSize chainSize)
    : OutputNodePrivate(context, chainSize)
{
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
