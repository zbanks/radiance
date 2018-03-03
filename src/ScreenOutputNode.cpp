#include "ScreenOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

ScreenOutputNode::ScreenOutputNode(Context *context, QSize chainSize)
    : OutputNode(new ScreenOutputNodePrivate(context, chainSize))
{
    d()->m_outputWindow = QSharedPointer<OutputWindow>(new OutputWindow(this));
    connect(d()->m_outputWindow.data(), &OutputWindow::screenNameChanged, this, &ScreenOutputNode::screenNameChanged);
    connect(d()->m_outputWindow.data(), &OutputWindow::shownChanged, this, &ScreenOutputNode::shownChanged);

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
