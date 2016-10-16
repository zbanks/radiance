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
    Q_OBJECT

public:
    LuxBus();
    LuxBus(QString uri);
    ~LuxBus();

    void setUri(QString uri);

public Q_SLOTS:
    void refresh();
    void sendFrames();
    void syncFrames();

private:
    int m_fd;
};
