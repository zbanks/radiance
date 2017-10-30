#pragma once

#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

class QQuickOutputWindow : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QString screen READ screen WRITE setScreen NOTIFY screenChanged)
    Q_PROPERTY(QStringList availableScreens READ availableScreens NOTIFY availableScreensChanged)
    Q_PROPERTY(QQuickWindow *window READ window CONSTANT)

protected:
    QSharedPointer<QQuickWindow> m_outputWindow;
    QScreen *m_screen;
    QList<QScreen *> m_screens;
    QStringList m_screenStrings;
    QTimer m_reloader;

    void putOnScreen();

protected slots:
    void onScreenChanged(QScreen* screen);
    void reload();

public:
    QQuickOutputWindow();
   ~QQuickOutputWindow();

public slots:
    void setScreen(QString screenName);
    QString screen();
    QStringList availableScreens();
    QQuickWindow *window();

signals:
    void screenChanged(QString screenName);
    void availableScreensChanged(QStringList availableScreens);
};
