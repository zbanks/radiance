#include "Output.h"
#include "Lux.h"

// OutputManager

OutputManager::OutputManager(QSettings * settings) : m_settings(settings) {
    loadSettings(settings);
}
OutputManager::OutputManager(QObject * p) {
    throw;
}

void OutputManager::loadSettings(QSettings * settings) {
    if (settings == nullptr)
        settings = m_settings;

    int size = settings->beginReadArray("buses");
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        // How to properly QVariant -> Enum?
        switch (settings->value("type").toInt()) {
        case OutputBus::Type::LuxBus:
            m_buses.append(new LuxBus(settings));
            break;
        default:
            throw;
        }
    }
    settings->endArray();

    size = settings->beginReadArray("devices");
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        // How to properly QVariant -> Enum?
        switch ((enum OutputBus::Type) settings->value("type").toInt()) {
        case OutputDevice::Type::LuxStripDevice:
            m_devices.append(new LuxStripDevice(settings));
            break;
        default:
            throw;
        }
    }
    settings->endArray();
}

void OutputManager::saveSettings(QSettings * settings) {
    if (settings == nullptr)
        settings = m_settings;
}

OutputManager::~OutputManager() {
    for (QList<QObject *>::iterator it = m_buses.begin(); it != m_buses.end(); it++) {
        delete *it;
    }
    for (QList<QObject *>::iterator it = m_devices.begin(); it != m_devices.end(); it++) {
        delete *it;
    }
}

QList<QObject *> OutputManager::buses() {
    return m_buses;
}

QList<QObject *> OutputManager::devices() {
    return m_devices;
}

void OutputManager::refresh() {

}

void OutputManager::createBus(OutputBus::Type type, QString uri) {
    switch (type) {
    case OutputBus::Type::LuxBus:
        LuxBus * luxbus = new LuxBus();
        luxbus->setUri(uri);
        m_buses.append(luxbus);
        emit busesChanged(m_buses);
        break;
    }
}
/*

void OutputManager::createDevice(OutputDevice::Type type, QString uri) {
    switch (type) {
    case LuxStrip:
        LuxStripDevice * luxstrip = new LuxStripDevice(this, uri);
        m_devices.append(luxstrip);
        emit devicesChanged(m_devices);
        break;
    }
}
*/

// OutputDevice
OutputDevice::State OutputDevice::state() {
    return m_state;
}
QString OutputDevice::name() {
    return m_name;
}
void OutputDevice::setName(QString name) {
    m_name = name;
    emit nameChanged(m_name);
}
QColor OutputDevice::color() {
    return m_color;
}
void OutputDevice::setColor(QColor color) {
    m_color = color;
    emit colorChanged(m_color);
}
QPolygonF OutputDevice::polygon() {
    return m_polygon;
}
void OutputDevice::setPolygon(QPolygonF polygon) {
    m_polygon = polygon;
    emit polygonChanged(m_polygon);
}
void OutputDevice::loadSettings(QSettings * settings) {
    setName(settings->value("name").toString());
    setColor(settings->value("color").value<QColor>());
    setPolygon(settings->value("polygon").value<QPolygonF>());
}
void OutputDevice::saveSettings(QSettings * settings) {
    settings->setValue("name", m_name);
    settings->setValue("color", m_color);
    settings->setValue("polygon", m_polygon);
}

// OutputBus
QString OutputBus::uri() {
    return m_uri;
}
OutputBus::State OutputBus::state() {
    return m_state;
}
