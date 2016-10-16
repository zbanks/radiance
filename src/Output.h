#pragma once

#include <QThread>

// what is C++

class OutputThread : public QThread {
public:
    OutputThread();
};

class OutputBus {
public:
    virtual ~OutputBus() {};
    virtual int refreshDevices() = 0;
    virtual void sendFrames() = 0;
    virtual void syncFrames() = 0;
};

class OutputDevice {
public:
    virtual ~OutputDevice() {};
    virtual void sendFrame() = 0;
    virtual void syncFrame() = 0;

    OutputBus * bus;
};

