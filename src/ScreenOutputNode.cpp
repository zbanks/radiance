#include "ScreenOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

ScreenOutputNode::ScreenOutputNode(Context *context, QSize chainSize)
    : OutputNode(context, chainSize)
    , m_outputWindow(nullptr) {
    m_outputWindow = new OutputWindow(this);

    connect(m_outputWindow, &OutputWindow::screenNameChanged, this, &ScreenOutputNode::screenNameChanged);
    connect(m_outputWindow, &OutputWindow::shownChanged, this, &ScreenOutputNode::shownChanged);

    reload();
    connect(&m_reloader, &QTimer::timeout, this, &ScreenOutputNode::reload);
    m_reloader.setInterval(1000); // Reload screens every 1000 ms
    m_reloader.start();
}

ScreenOutputNode::ScreenOutputNode(const ScreenOutputNode &other)
    : OutputNode(other) {
}

ScreenOutputNode::~ScreenOutputNode() {
    delete m_outputWindow;
}

void ScreenOutputNode::setShown(bool shown) {
    m_outputWindow->setShown(shown);
}

bool ScreenOutputNode::shown() {
    return m_outputWindow->shown();
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
