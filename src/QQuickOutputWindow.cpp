#include "QQuickOutputWindow.h"
#include "main.h"

#include <QScreen>
#include <QGuiApplication>

// QQuickOutputWindow

QQuickOutputWindow::QQuickOutputWindow()
    : m_screen(nullptr) {
    m_outputWindow = QSharedPointer<QQuickWindow>(new QQuickWindow());
    m_screen = m_outputWindow->screen();
    connect(m_outputWindow.data(), &QWindow::screenChanged, this, &QQuickOutputWindow::onScreenChanged);

    m_outputWindow->setFlags(Qt::Dialog);
    m_outputWindow->setWindowState(Qt::WindowFullScreen);
    putOnScreen();
    connect(m_outputWindow.data(), &QWindow::screenChanged, this, &QQuickOutputWindow::putOnScreen);

    reload();
    connect(&m_reloader, &QTimer::timeout, this, &QQuickOutputWindow::reload);
    m_reloader.setInterval(1000); // Reload MIDI every 1000 ms
    m_reloader.start();
}

QQuickOutputWindow::~QQuickOutputWindow() {
}

void QQuickOutputWindow::putOnScreen() {
    m_outputWindow->setGeometry(m_outputWindow->screen()->geometry());
}

QString QQuickOutputWindow::screen() {
    return m_outputWindow->screen()->name();
}

void QQuickOutputWindow::onScreenChanged(QScreen *screen) {
    reload();
    if(screen != m_screen) {
        if (m_screens.contains(screen)) {
            m_outputWindow->setScreen(m_screen);
        } else {
            qDebug() << "Screen " << m_screen->name() << " disappeared!";
        }
    }
}

void QQuickOutputWindow::setScreen(QString screenName) {
    reload();
    foreach(QScreen *screen, m_screens) {
        if(screen->name() == screenName) {
            m_screen = screen;
            m_outputWindow->setScreen(screen);
            emit screenChanged(screen->name());
            break;
        }
    }
}

QStringList QQuickOutputWindow::availableScreens() {
    return m_screenStrings;
}

QQuickWindow *QQuickOutputWindow::window() {
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
}
