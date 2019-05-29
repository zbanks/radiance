#pragma once

#include "OutputNode.h"
#include <QOpenGLWindow>
#include <QTimer>
#include <QOpenGLShaderProgram>

class OutputWindow : public QOpenGLWindow {
    Q_OBJECT

protected:
    QString m_screenName;
    QTimer m_reloader;
    bool m_found;
    QOpenGLShaderProgram *m_program;
    QSharedPointer<OutputNode> m_videoNode;
    QOpenGLVertexArrayObject m_vao;
    bool m_shown;
    QSize m_screenSize;

    void putOnScreen();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *ev) override;
    bool event(QEvent *e) override;

protected slots:
    void onScreenChanged(QScreen* screen);
    void reload();
    void setScreenSize(QSize screenSize);

public:
    OutputWindow(QSharedPointer<OutputNode> videoNode);

public slots:
    void setScreenName(QString screen);
    QString screenName();
    bool found();
    void setShown(bool shown);
    bool shown();
    QSize screenSize();

signals:
    void screenNameChanged(QString screenName);
    void availableScreensChanged(QStringList availableScreens);
    void foundChanged(bool found);
    void shownChanged(bool shown);
    void screenSizeChanged(QSize screenSize);
};
