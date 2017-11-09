#include "QQuickOutputWindow.h"
#include "main.h"

#include <QScreen>
#include <QGuiApplication>

// QQuickOutputWindow

QQuickOutputWindow::QQuickOutputWindow()
    : m_screen("")
    , m_found(false) {
    m_outputWindow = QSharedPointer<QQuickWindow>(new QQuickWindow());
    connect(m_outputWindow.data(), &QWindow::screenChanged, this, &QQuickOutputWindow::onScreenChanged);

    m_outputWindow->setFlags(Qt::Dialog);
    m_outputWindow->setWindowState(Qt::WindowFullScreen);
    putOnScreen();
    connect(m_outputWindow.data(), &QWindow::screenChanged, this, &QQuickOutputWindow::putOnScreen);

    reload();
    connect(&m_reloader, &QTimer::timeout, this, &QQuickOutputWindow::reload);
    m_reloader.setInterval(1000); // Reload screens every 1000 ms
    m_reloader.start();
}

QQuickOutputWindow::~QQuickOutputWindow() {
}

void QQuickOutputWindow::putOnScreen() {
    m_outputWindow->setGeometry(m_outputWindow->screen()->geometry());
}

QString QQuickOutputWindow::screen() {
    return m_screen;
}

void QQuickOutputWindow::onScreenChanged(QScreen *screen) {
    reload();
}

void QQuickOutputWindow::setScreen(QString screen) {
    if (screen != m_screen) {
        m_screen = screen;
        emit screenChanged(m_screen);
        reload();
    }
}

QStringList QQuickOutputWindow::availableScreens() {
    return m_screenStrings;
}

QQuickWindow *QQuickOutputWindow::outputWindow() {
    return m_outputWindow.data();
}

void QQuickOutputWindow::reload() {
    auto screens = QGuiApplication::screens();

    if (screens != m_screens) {
        m_screens = screens;

        m_screenStrings.clear();

        foreach(QScreen *screen, m_screens) {
            m_screenStrings << screen->name();
        }

        emit availableScreensChanged(m_screenStrings);
    }

    bool found = false;

    foreach(QScreen *screen, m_screens) {
        if (screen->name() == m_screen) {
            if (m_outputWindow->screen() != screen) {
                m_outputWindow->setScreen(screen);
            }
            found = true;
        }
    }

    if (found != m_found) {
        m_found = found;
        emit foundChanged(found);
    }
}

bool QQuickOutputWindow::found() {
    return m_found;
}
