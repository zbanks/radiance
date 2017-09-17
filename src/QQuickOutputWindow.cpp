#include "QQuickOutputWindow.h"
#include "main.h"

#include <QScreen>
#include <QGuiApplication>

// QQuickOutputWindow

QQuickOutputWindow::QQuickOutputWindow()
    : m_screen(0) {
    m_outputWindow = QSharedPointer<QQuickWindow>(new QQuickWindow());
    m_screen = m_outputWindow->screen();
    connect(m_outputWindow.data(), &QWindow::screenChanged, this, &QQuickOutputWindow::onScreenChanged);

    m_outputWindow->setFlags(Qt::Dialog);
    m_outputWindow->setWindowState(Qt::WindowFullScreen);
    putOnScreen();
    connect(m_outputWindow.data(), &QWindow::screenChanged, this, &QQuickOutputWindow::putOnScreen);
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
    if(screen != m_screen) {
        m_outputWindow->setScreen(m_screen);
    }
}

void QQuickOutputWindow::setScreen(QString screenName) {
    foreach(QScreen *screen, QGuiApplication::screens()) {
        if(screen->name() == screenName) {
            m_screen = screen;
            m_outputWindow->setScreen(screen);
            emit screenChanged(screen->name());
            break;
        }
    }
}

QStringList QQuickOutputWindow::availableScreens() {
    QStringList result;
    foreach(QScreen *screen, QGuiApplication::screens()) {
        result.append(screen->name());
    }
    return result;
}

QQuickWindow *QQuickOutputWindow::window() {
    return m_outputWindow.data();
}
