#include "Lux.h"
#include "liblux/lux.h"
#include <unistd.h>

// TODO: Use QtSerialPort & QtUdpSocket instead of file descriptors
// to make lux platform-indepdendent
LuxBus::LuxBus() : m_fd(-1) {

}

LuxBus::LuxBus(QString uri) : m_fd(-1) {

}

LuxBus::~LuxBus() {
    if (m_fd > 0)
        lux_close(m_fd);
}

/*
LuxBus::LuxBus(std::string uri) : uri(uri) {
    this->uri = uri;
    fd = lux_uri_open(uri.c_str());
    if (fd < 0)
        throw std::exception();
}
*/

void LuxBus::setUri(QString uri) {
    m_uri = uri;
    emit uriChanged(m_uri);

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

void LuxBus::sendFrames() {

}

void LuxBus::syncFrames() {

}
