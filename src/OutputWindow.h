#pragma once

#include <QWindow>
#include <QTimer>

class OutputWindow : public QWindow {
    Q_OBJECT
    Q_PROPERTY(QString screenName READ screenName WRITE setScreenName NOTIFY screenNameChanged)
    Q_PROPERTY(bool found READ found NOTIFY foundChanged)

protected:
    QString m_screenName;
    QTimer m_reloader;
    bool m_found;

    void putOnScreen();

protected slots:
    void onScreenChanged(QScreen* screen);
    void reload();

public:
    OutputWindow();
   ~OutputWindow();

public slots:
    void setScreenName(QString screen);
    QString screenName();
    bool found();

signals:
    void screenNameChanged(QString screenName);
    void availableScreensChanged(QStringList availableScreens);
    void foundChanged(bool found);
};
