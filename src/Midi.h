#pragma once

#include <vector>
#include <cstdint>
#include <QObject>
#include <QByteArray>
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

    int deviceIndex() const;
    void setDeviceIndex(int idx);

    virtual void callback(double ts, std::vector<uint8_t> &message);

public slots:
    void reload();

    void onRealtimeEvent(double ts, int status);
    void onNormalEvent  (double ts, int status, int val0, int val1);
signals:
    void realtimeEvent (double ts, int status);
    void normalEvent   (double ts, int status, int val0, int val1);
    void rtClock (double ts);
    void rtStart (double ts);
    void rtContinue(double ts);
    void rtStop(double ts);
    void rtActiveSensing(double ts);
    void rtReset(double ts);
    void noteOff       (double ts, int channel, int note, int velocity);
    void noteOn        (double ts, int channel, int note, int velocity);
    void noteAftertouch(double ts, int channel, int note, int velocity);
    void controlChange (double ts, int channel, int control, int value);
    void programChange (double ts, int channel, int patch);
    void channelAftertouch(double ts,int channel, int velocity);
    void pitchBend(double ts, int channel, int bend);
    void sysExEvent   (double ts, QByteArray data);
    void deviceListChanged(QStringList deviceList);
    void deviceIndexChanged(int idx);
protected:
    double m_lastTime{};
    QStringList m_deviceList;
    int m_deviceIndex;
    RtMidiIn m_midiin;
    bool     m_inSysEx;
    QByteArray m_sysEx;
};
