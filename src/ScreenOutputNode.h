#pragma once

#include "OutputNode.h"
#include "OutputWindow.h"
#include <QScreen>

class ScreenOutputNode
    : public OutputNode {
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged);
    Q_PROPERTY(QStringList availableScreens READ availableScreens NOTIFY availableScreensChanged);

public:
    ScreenOutputNode(Context *context, QSize chainSize);
    ScreenOutputNode(const ScreenOutputNode &other);
    ~ScreenOutputNode();

public slots:
    bool visible();
    void setVisible(bool visible);
    QStringList availableScreens();

signals:
    void visibleChanged(bool visible);
    void availableScreensChanged(QStringList availableScreens);

protected:
    OutputWindow *m_outputWindow;

protected slots:
    void reload();

private:
    QList<QScreen *> m_screens;
    QStringList m_screenNameStrings;
    QTimer m_reloader;
};

//////////////////////////////////////////////////////////////////////////////

class ScreenOutputNodeFactory : public VideoNodeFactory {

public:
    QString typeName() override;
    VideoNode *deserialize(Context *context, QJsonObject obj) override;
};
