#pragma once

#include "VideoNodeUI.h"
#include <QQuickFramebufferObject>

class OutputWindow;

class OutputUI : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(VideoNodeUI *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QString screen READ screen WRITE setScreen NOTIFY screenChanged)
    Q_PROPERTY(QStringList availableScreens READ availableScreens NOTIFY availableScreensChanged)

protected:
    OutputWindow *m_outputWindow;
    QScreen *m_screen;

protected slots:
    void onScreenChanged(QScreen* screen);
    void onVisibleChanged(bool visible);

public:
    VideoNodeUI *source();
    void setSource(VideoNodeUI *);
    OutputUI();
   ~OutputUI();
    VideoNodeUI *m_source;
    QMutex m_sourceLock;

public slots:
    void setVisible(bool visible);
    bool visible();
    void setScreen(QString screenName);
    QString screen();
    QStringList availableScreens();

signals:
    void sourceChanged(VideoNodeUI *value);
    void visibleChanged(bool value);
    void screenChanged(QString screenName);
    void availableScreensChanged();
};
