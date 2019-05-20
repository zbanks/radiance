#pragma once

#include "OutputNode.h"
#include "OutputWindow.h"
#include <QList>
#include <QSize>
#include <QScreen>

class ScreenOutputNode
    : public OutputNode {
    Q_OBJECT
    Q_PROPERTY(bool shown READ shown WRITE setShown NOTIFY shownChanged);
    Q_PROPERTY(bool found READ found NOTIFY foundChanged);
    Q_PROPERTY(QStringList availableScreens READ availableScreens NOTIFY availableScreensChanged);
    Q_PROPERTY(QString screenName READ screenName WRITE setScreenName NOTIFY screenNameChanged);
    Q_PROPERTY(QVariantList suggestedResolutions READ suggestedResolutions NOTIFY suggestedResolutionsChanged);
    Q_PROPERTY(QSize resolution READ resolution WRITE setResolution NOTIFY resolutionChanged);

public:
    ScreenOutputNode(Context *context, QSize chainSize);

    // These static methods are required for VideoNode creation
    // through the registry

    // A string representation of this VideoNode type
    static QString typeName();

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    static VideoNodeSP *deserialize(Context *context, QJsonObject obj);

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    static bool canCreateFromFile(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    static VideoNodeSP *fromFile(Context *context, QString filename);

    // Returns QML filenames that can be loaded
    // to instantiate custom instances of this VideoNode
    static QMap<QString, QString> customInstantiators();

    static QList<QSize> commonResolutions;

public slots:
    bool shown();
    void setShown(bool shown);
    bool found();
    QStringList availableScreens();
    QString screenName();
    void setScreenName(QString screenName);
    QSize resolution();
    void setResolution(QSize resolution);
    QVariantList suggestedResolutions();

signals:
    void shownChanged(bool shown);
    void foundChanged(bool found);
    void availableScreensChanged(QStringList availableScreens);
    void screenNameChanged(QString screenName);
    void resolutionChanged(QSize resolution);
    void suggestedResolutionsChanged(QVariantList resolutions);

protected slots:
    void reload();
    void onScreenSizeChanged(QSize screenSize);

protected:
    ScreenOutputNodePrivate(Context *context, QSize chainSize);
    QList<QScreen *> m_screens;
    QStringList m_screenNameStrings;
    QTimer m_reloader;
    QList<QSize> m_suggestedResolutions;

    // Not actually shared, just convenient for deletion
    QSharedPointer<OutputWindow> m_outputWindow;
};

typedef QmlSharedPointer<ScreenOutputNode> ScreenOutputNodeSP;
Q_DECLARE_METATYPE(ScreenOutputNodeSP*)
