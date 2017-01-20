#include "Lux.h"
#include "liblux/lux.h"
#include <unistd.h>

quint32 LuxDevice::luxId() {
    return m_id;
}
void LuxDevice::setLuxId(quint32 id) {
    if (id != m_id) {
        m_id = id;
        m_state = Disconnected;
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
    setLength(settings->value("length").toInt());
    setLuxId(settings->value("luxId").value<quint32>());
    OutputDevice::loadSettings(settings);
}
void LuxDevice::saveSettings(QSettings * settings) {
    settings->setValue("name", m_name);
    settings->setValue("color", m_color);
    settings->setValue("polygon", m_polygon);
    OutputDevice::saveSettings(settings);
}

//
LuxStripDevice::LuxStripDevice(QSettings * settings) {
}
LuxStripDevice::~LuxStripDevice() {
}
void LuxStripDevice::frame() {
}
void LuxStripDevice::refresh() {
}

//

// TODO: Use QtSerialPort & QtUdpSocket instead of file descriptors
// to make lux platform-indepdendent
LuxBus::LuxBus() : m_fd(-1) {
    //m_state = Disconnected;
}
LuxBus::LuxBus(QSettings * settings) : m_fd(-1) {
    //m_state = Disconnected;
    setUri(settings->value("uri").toString());
}

LuxBus::~LuxBus() {
    if (m_fd > 0)
        lux_close(m_fd);
}

void LuxBus::setUri(QString uri) {
    m_uri = uri;
    //emit uriChanged(m_uri);

    if (m_fd >= 0)
        lux_close(m_fd);
    m_state = Disconnected;
    emit stateChanged(m_state);

    m_fd = lux_uri_open(m_uri.toStdString().c_str());
    if (m_fd < 0)
        m_state = Error;
    else
        m_state = Connected;
    emit stateChanged(m_state);
}

void LuxBus::refresh() {

}

void LuxBus::beginFrame() {
    /*
    for (QVector<OutputDevice *>::Iterator it = m_devices.begin(); it != m_devices.end(); it++) {
        it->sendFrames();
    }
    */
}

void LuxBus::endFrame() {
}

void LuxBus::detectDevices(QVector<OutputDevice *> device_hints) {

}
