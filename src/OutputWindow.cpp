#include "OutputWindow.h"

#include <QScreen>
#include <QGuiApplication>

// OutputWindow

OutputWindow::OutputWindow()
    : m_screenName("")
    , m_found(false) {
    connect(this, &QWindow::screenChanged, this, &OutputWindow::onScreenChanged);

    setFlags(Qt::Dialog);
    setWindowState(Qt::WindowFullScreen);
    putOnScreen();
    connect(this, &QWindow::screenChanged, this, &OutputWindow::putOnScreen);

    reload();
    connect(&m_reloader, &QTimer::timeout, this, &OutputWindow::reload);
    m_reloader.setInterval(1000); // Reload screens every 1000 ms
    m_reloader.start();
}

OutputWindow::~OutputWindow() {
}

void OutputWindow::putOnScreen() {
    setGeometry(screen()->geometry());
}

QString OutputWindow::screenName() {
    return m_screenName;
}

void OutputWindow::onScreenChanged(QScreen *screen) {
    reload();
}

void OutputWindow::setScreenName(QString screenName) {
    if (screenName != m_screenName) {
        m_screenName = screenName;
        emit screenNameChanged(m_screenName);
        reload();
    }
}

void OutputWindow::reload() {
    auto screens = QGuiApplication::screens();

    bool found = false;

    foreach(QScreen *testScreen, screens) {
        if (testScreen->name() == m_screenName) {
            if (screen() != testScreen) {
                setScreen(testScreen);
            }
            found = true;
        }
    }

    if (found != m_found) {
        m_found = found;
        emit foundChanged(found);
    }
}

bool OutputWindow::found() {
    return m_found;
}
