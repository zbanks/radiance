#include "Lux.h"
#include "liblux/lux.h"
#include <unistd.h>

// TODO: Use QtSerialPort & QtUdpSocket instead of file descriptors
// to make lux platform-indepdendent

LuxBus::LuxBus(std::string uri) {
    this->uri = uri;
    fd = lux_uri_open(uri.c_str());
    if (fd < 0)
        throw std::exception();
}

LuxBus::LuxBus(const LuxBus &bus) {
    uri = bus.uri;
    fd = dup(bus.fd);
}

LuxBus::~LuxBus() {
    lux_close(fd);
}

int LuxBus::refreshDevices() {

}

void LuxBus::sendFrames() {

}

void LuxBus::syncFrames() {

}
