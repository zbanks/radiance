#pragma once

#include <vector>
#include <cstdint>
#include <QObject>
#include <QByteArray>
#include <QQuickItem>
#include <QTimer>
#include <rtmidi/RtMidi.h>

class MidiController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList deviceList READ deviceList NOTIFY deviceListChanged);
    Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged);
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged);

public:
    MidiController(QObject *p = nullptr);
    ~MidiController() override;

    virtual void callback(double ts, std::vector<uint8_t> &message);
    virtual void errorCallback(RtMidiError::Type type, const std::string &errorText);

public slots:
    QStringList deviceList();
    QString deviceName();
    void setDeviceName(QString name);
    bool connected();

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
    void deviceNameChanged(QString deviceName);
    void connectedChanged(bool connected);

protected slots:
    void onRealtimeEvent(double ts, int status);
    void onNormalEvent  (double ts, int status, int val0, int val1);
    void reload();
    void reloadDevice();

protected:
    double m_lastTime{};
    QStringList m_deviceList;
    RtMidiIn m_midiin;
    bool     m_inSysEx;
    QByteArray m_sysEx;
    QString m_deviceName;
    QTimer m_reloader;
    bool m_connected;
};
