#pragma once

#include "OutputNode.h"
#include "OutputWindow.h"
#include <QScreen>

class ScreenOutputNode
    : public OutputNode {
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged);
    Q_PROPERTY(QStringList availableScreens READ availableScreens NOTIFY availableScreensChanged);
    Q_PROPERTY(QString screenName READ screenName WRITE setScreenName NOTIFY screenNameChanged);

public:
    ScreenOutputNode(Context *context, QSize chainSize);
    ScreenOutputNode(const ScreenOutputNode &other);
    ~ScreenOutputNode();

    // These static methods are required for VideoNode creation
    // through the registry

    // A string representation of this VideoNode type
    static QString typeName();

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    static VideoNode *deserialize(Context *context, QJsonObject obj);

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    static bool canCreateFromFile(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    static VideoNode *fromFile(Context *context, QString filename);

    // Returns QML filenames that can be loaded
    // to instantiate custom instances of this VideoNode
    static QMap<QString, QString> customInstantiators();

public slots:
    bool visible();
    void setVisible(bool visible);
    QStringList availableScreens();
    QString screenName();
    void setScreenName(QString screenName);

signals:
    void visibleChanged(bool visible);
    void availableScreensChanged(QStringList availableScreens);
    void screenNameChanged();

protected:
    OutputWindow *m_outputWindow;

protected slots:
    void reload();

private:
    QList<QScreen *> m_screens;
    QStringList m_screenNameStrings;
    QTimer m_reloader;
};
