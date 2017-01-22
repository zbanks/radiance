#include "Lux.h"
#include "liblux/lux.h"
#include <unistd.h>

LuxDevice::LuxDevice(QQuickItem * parent) :
    QQuickItem(parent),
    m_state(State::Disconnected) {
}
LuxDevice::State LuxDevice::state() {
    return m_state;
}
QString LuxDevice::name() {
    return m_name;
}
void LuxDevice::setName(QString name) {
    m_name = name;
    emit nameChanged(m_name);
}
QColor LuxDevice::color() {
    return m_color;
}
void LuxDevice::setColor(QColor color) {
    m_color = color;
    emit colorChanged(m_color);
}
QPolygonF LuxDevice::polygon() {
    return m_polygon;
}
void LuxDevice::setPolygon(QPolygonF polygon) {
    m_polygon = polygon;
    emit polygonChanged(m_polygon);
}
quint32 LuxDevice::luxId() {
    return m_id;
}
void LuxDevice::setLuxId(quint32 id) {
    if (id != m_id) {
        m_id = id;
        m_state = State::Disconnected;
        emit luxIdChanged(m_id);
        emit stateChanged(m_state);
    }
}
int LuxDevice::length() {
    return m_length;
}
void LuxDevice::setLength(int length) {
    m_length = length;
    emit lengthChanged(m_length);
}
void LuxDevice::loadSettings(QSettings * settings) {
    setName(settings->value("name").toString());
    setColor(settings->value("color").value<QColor>());
    setPolygon(settings->value("polygon").value<QPolygonF>());
    setLength(settings->value("length").toInt());
    setLuxId(settings->value("luxId").value<quint32>());
}
void LuxDevice::saveSettings(QSettings * settings) {
    settings->setValue("name", m_name);
    settings->setValue("color", m_color);
    settings->setValue("polygon", m_polygon);
}

void LuxDevice::frame() {
}
void LuxDevice::refresh() {
}

//

// TODO: Use QtSerialPort & QtUdpSocket instead of file descriptors
// to make lux platform-indepdendent
LuxBus::LuxBus(QQuickItem * parent) :
    QQuickItem(parent),
    m_state(State::Disconnected),
    m_fd(-1) {
}

LuxBus::~LuxBus() {
    if (m_fd > 0)
        lux_close(m_fd);
}

QString LuxBus::uri() {
    return m_uri;
}
LuxBus::State LuxBus::state() {
    return m_state;
}
void LuxBus::setUri(QString uri) {
    m_uri = uri;

    if (m_fd >= 0) {
        lux_close(m_fd);
        m_fd = -1;
        m_state = State::Disconnected;
        emit stateChanged(m_state);
    }

    if (!m_uri.isEmpty()) {
        m_fd = lux_uri_open(m_uri.toStdString().c_str());
        if (m_fd < 0)
            m_state = State::Error;
        else
            m_state = State::Connected;
        emit stateChanged(m_state);
    }
    emit uriChanged(m_uri);
}

void LuxBus::loadSettings(QSettings * settings) {
    setUri(settings->value("uri").toString());
}
void LuxBus::saveSettings(QSettings * settings) {
    settings->setValue("uri", m_uri);
}

void LuxBus::refresh() {

}

void LuxBus::beginFrame() {
    /*
    for (auto dev : m_devices) {
        it->sendFrames();
    }
    */
}

void LuxBus::endFrame() {
}

void LuxBus::detectDevices(QList<LuxDevice *> device_hints) {

}
