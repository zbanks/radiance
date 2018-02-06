#include "ScreenOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

ScreenOutputNode::ScreenOutputNode(Context *context, QSize chainSize)
    : OutputNode(context, chainSize)
    , m_outputWindow(nullptr) {
    m_outputWindow = new OutputWindow(this);

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

void ScreenOutputNode::setVisible(bool visible) {
    m_outputWindow->setVisible(visible);
    emit visibleChanged(visible);
}

bool ScreenOutputNode::visible() {
    return m_outputWindow->isVisible();
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
    qDebug() << m_screenNameStrings;
    return m_screenNameStrings;
}

QString ScreenOutputNode::typeName() {
    return "ScreenOutputNode";
}

VideoNode *ScreenOutputNode::deserialize(Context *context, QJsonObject obj) {
    ScreenOutputNode *e = new ScreenOutputNode(context, QSize(500, 500));
    return e;
}

bool ScreenOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *ScreenOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}
