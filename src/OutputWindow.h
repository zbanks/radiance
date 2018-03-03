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
    QScopedPointer<OutputNode> m_videoNode;
    QOpenGLVertexArrayObject m_vao;
    bool m_shown;

    void putOnScreen();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *ev) override;
    bool event(QEvent *e) override;

protected slots:
    void onScreenChanged(QScreen* screen);
    void reload();

public:
    // The VideoNode that this is constructed with
    // will be cloned and the clone will be destroyed
    // when the OutputWindow is destroyed.
    // You do not need to worry about the lifetime
    // of the VideoNode* that you pass in.
    OutputWindow(OutputNode *videoNode);

public slots:
    void setScreenName(QString screen);
    QString screenName();
    bool found();
    void setShown(bool shown);
    bool shown();

signals:
    void screenNameChanged(QString screenName);
    void availableScreensChanged(QStringList availableScreens);
    void foundChanged(bool found);
    void shownChanged(bool shown);
};
