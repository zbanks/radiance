#pragma once

#include <QQuickItem>
#include <QSettings>
#include <QThread>
#include <vector>

#include "Lux.h"

// OutputManager singleton
class OutputManager : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<LuxBus> buses READ buses NOTIFY busesChanged)
    Q_PROPERTY(QQmlListProperty<LuxDevice> devices READ devices NOTIFY devicesChanged)

public:
    OutputManager(QSettings * settings, QQuickItem * p = nullptr);

    Q_INVOKABLE void saveSettings(QSettings * settings = nullptr);

    QQmlListProperty<LuxBus> buses();
    Q_INVOKABLE LuxBus * createLuxBus(QString uri = 0);

    QQmlListProperty<LuxDevice> devices();
    Q_INVOKABLE LuxDevice * createLuxDevice();

public slots:
    void refresh();

signals:
    void busesChanged(QList<LuxBus *>);
    void devicesChanged(QList<LuxDevice *>);

private:
    // not sure if this should be public
    void loadSettings(QSettings * settings);


    QList<LuxBus*> m_buses;
    QList<LuxDevice *> m_devices;
    QSettings * m_settings;
};

