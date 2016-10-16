#pragma once
#include "Output.h"
#include <vector>
#include <string>

class LuxBus;

class LuxDevice : public OutputDevice {
public:
    LuxDevice(LuxBus * bus);
    LuxDevice(const LuxDevice &dev);
    ~LuxDevice() {};
    void sendFrame();
    void syncFrame();
};


class LuxBus : public OutputBus {
public:
    LuxBus(std::string uri);
    LuxBus(const LuxBus &bus);
    ~LuxBus();

    int refreshDevices();
    void sendFrames();
    void syncFrames();

private:
    std::vector<LuxDevice> devices;
    std::string uri;
    int fd;
};
