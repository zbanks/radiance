#pragma once

#include <QObject>
#include <QQuickItem>
#include <QTimer>
#include <rtmidi/RtMidi.h>

class MidiDevice : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QStringList deviceList READ deviceList NOTIFY deviceListChanged); 
    Q_PROPERTY(int deviceIndex READ deviceIndex WRITE setDeviceIndex NOTIFY deviceIndexChanged);

public:
    MidiDevice(QQuickItem *p = nullptr);
    ~MidiDevice() override;

    QStringList deviceList();

    int deviceIndex();
    void setDeviceIndex(int idx);

public Q_SLOTS:
    void reload();
    void poll();

signals:
    void controlChangeEvent(int channel, int control, int value);
    void noteOnEvent(int channel, int note, int velocity);
    void noteOffEvent(int channel, int note, int velocity);
    void deviceListChanged(QStringList deviceList);
    void deviceIndexChanged(int idx);

private:
    QStringList m_deviceList;
    int m_deviceIndex;
    RtMidiIn m_midiin;
    QTimer m_timer;
};
