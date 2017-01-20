#pragma once
#include "Output.h"
#include <vector>
#include <string>

class LuxBus;

class LuxDevice : public OutputDevice {
    friend class LuxBus;
    Q_OBJECT;
    Q_PROPERTY(quint32 luxId READ luxId WRITE setLuxId NOTIFY luxIdChanged)
    Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
public:
    ~LuxDevice() {};

    void loadSettings(QSettings * settings);
    void saveSettings(QSettings * settings);

    quint32 luxId();
    void setLuxId(quint32 luxId);
    int length();
    void setLength(int length);

signals:
    void luxIdChanged(quint32 luxId);
    void lengthChanged(int length);

protected:
    int m_length;

    LuxBus * m_bus;
    quint32 m_id;
};

class LuxStripDevice : public LuxDevice {
public:
    LuxStripDevice(QSettings * settings);
    LuxStripDevice();
    ~LuxStripDevice();

    void frame();
    void refresh();
};


class LuxBus : public OutputBus {
    friend class LuxDevice;
    Q_OBJECT

public:
    LuxBus();
    LuxBus(QSettings * settings);
    ~LuxBus();

    void beginFrame();
    void endFrame();
    void detectDevices(QVector<OutputDevice *> device_hints);

    void setUri(QString uri);

public Q_SLOTS:
    void refresh();

protected:
    int m_fd;
};
