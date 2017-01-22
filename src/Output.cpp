#include "Output.h"
#include "Lux.h"

// OutputManager

OutputManager::OutputManager(QSettings * settings, QQuickItem * parent) :
    QQuickItem(parent),
    m_settings(settings) {
    loadSettings(settings);
}
void OutputManager::loadSettings(QSettings * settings) {
    int size = settings->beginReadArray("buses");
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        LuxBus * luxbus = new LuxBus(this);
        luxbus->loadSettings(settings);
        m_buses.append(luxbus);
    }
    settings->endArray();

    size = settings->beginReadArray("devices");
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        LuxDevice * luxdev = new LuxDevice(this);
        luxdev->loadSettings(settings);
        m_devices.append(luxdev);
    }
    settings->endArray();
}

void OutputManager::saveSettings(QSettings * settings) {
    if (settings != nullptr)
        m_settings = settings;

    m_settings->beginWriteArray("buses", m_buses.size());
    int i = 0;
    for (auto bus : m_buses) {
        m_settings->setArrayIndex(i++);
        bus->saveSettings(m_settings);
    }
    m_settings->endArray();

    m_settings->beginWriteArray("devices", m_devices.size());
    i = 0;
    for (auto device : m_devices) {
        m_settings->setArrayIndex(i++);
        device->saveSettings(m_settings);
    }
    m_settings->endArray();
}

QQmlListProperty<LuxBus> OutputManager::buses() {
    // XXX not production; should replace with count/at fns
    return QQmlListProperty<LuxBus>(this, m_buses);
}

QQmlListProperty<LuxDevice> OutputManager::devices() {
    // XXX not production; should replace with count/at fns
    return QQmlListProperty<LuxDevice>(this, m_devices);
}

void OutputManager::refresh() {

}

LuxBus * OutputManager::createLuxBus(QString uri) {
    LuxBus * luxbus = new LuxBus(this);
    luxbus->setUri(uri);
    m_buses.append(luxbus);
    emit busesChanged(m_buses);
    return luxbus;
}

LuxDevice * OutputManager::createLuxDevice() {
    LuxDevice * luxdev = new LuxDevice(this);
    m_devices.append(luxdev);
    emit devicesChanged(m_devices);
    return luxdev;
}
