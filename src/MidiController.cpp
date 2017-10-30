#include "MidiController.h"

MidiController::MidiController(QObject* parent) :
    QObject(parent)
{
    connect(this, &MidiController::normalEvent, this, &MidiController::onNormalEvent, Qt::QueuedConnection);
    connect(this, &MidiController::realtimeEvent, this, &MidiController::onRealtimeEvent, Qt::QueuedConnection);
    connect(&m_reloader, &QTimer::timeout, this, &MidiController::reload);

    reload();
    m_reloader.setInterval(1000); // Reload MIDI every 1000 ms
    m_reloader.start();
}

MidiController::~MidiController() {
    if (m_midiin.isPortOpen()) {
        m_midiin.cancelCallback();
        m_midiin.closePort();
    }
}

void MidiController::onRealtimeEvent(double ts, int status) {
    switch (status) {
        case 0xf8: emit rtClock(ts); break;
        case 0xfa: emit rtStart(ts); break;
        case 0xfb: emit rtContinue(ts); break;
        case 0xfc: emit rtStop(ts); break;
        case 0xfe: emit rtActiveSensing(ts); break;
        case 0xff: emit rtReset(ts); break;
        default: break;
    }
}

void MidiController::onNormalEvent(double ts, int status, int val0, int val1) {
    auto ch = status & 0x0f;
    switch ((status >> 4) & 7) {
        case 0: emit noteOff(ts, ch, val0, val1); break;
        case 1: emit noteOn(ts, ch, val0, val1); break;
        case 2: emit noteAftertouch(ts, ch, val0, val1); break;
        case 3: emit controlChange(ts, ch, val0, val1); break;
        case 4: emit programChange(ts, ch, val0); break;
        case 5: emit channelAftertouch(ts, ch, val0); break;
        case 6: emit pitchBend(ts, ch, ((val0 & 0x7f) | ((val1 & 0x7f) << 7)) - 0x2000); break;
        default: break;
    }
}

QStringList MidiController::deviceList() {
    return m_deviceList;
}

void MidiController::reloadDevice() {
    if (m_midiin.isPortOpen()) {
        m_midiin.cancelCallback();
        m_midiin.closePort();
    }
    m_inSysEx = false;
    reload();
}

void MidiController::reload() {
    auto dl = QStringList();
    unsigned int count = m_midiin.getPortCount();
    for (unsigned int i = 0; i < count; i++) {
        auto deviceName = QString::fromStdString(m_midiin.getPortName(i));
        dl << deviceName;
        if (!m_midiin.isPortOpen() && !m_deviceName.isEmpty() && deviceName.contains(m_deviceName)) {
            auto cbTrampoline = [](double ts, std::vector<unsigned char> *msg, void *opaque) {
                if (opaque != nullptr && msg != nullptr) static_cast<MidiController*>(opaque)->callback(ts, *msg);
            };
            auto errorCbTrampoline = [](RtMidiError::Type type, const std::string &errorText, void *opaque) {
                if (opaque != nullptr) static_cast<MidiController*>(opaque)->errorCallback(type, errorText);
            };
            m_midiin.setCallback(static_cast<RtMidiIn::RtMidiCallback>(cbTrampoline), static_cast<void*>(this));
            m_midiin.setErrorCallback(static_cast<RtMidiErrorCallback>(errorCbTrampoline), static_cast<void*>(this));
            m_midiin.openPort(i, "Radiance Controller");
        }
    }
    if (m_deviceList != dl) {
        m_deviceList = dl;
        reloadDevice(); // TODO this causes re-enumeration, may want to change this to be one-pass
        emit deviceListChanged(m_deviceList);
    }
    auto open = m_midiin.isPortOpen();
    if (m_connected != open) {
        m_connected = open;
        emit connectedChanged(m_connected);
    }
}

void MidiController::errorCallback(RtMidiError::Type type, const std::string &errorText) {
    qDebug() << "RTMIDI ERROR" << QString::fromStdString(errorText);
}

void MidiController::callback(double ts, std::vector<uint8_t>&msg) {
    m_lastTime += ts;
    ts = m_lastTime;
    if (msg.empty()) return;
    auto status = int(msg.front());
    if ((status & 0xF8) == 0xF8) {
        realtimeEvent(ts, status);
        return;
    }
    if (!m_inSysEx) {
        if (status == 0xF0) {
            status = 0;
            m_inSysEx = true;
        } else {
            auto val0 = 0, val1 = 0;
            switch(msg.size()) {
                default:
                case 3: val1 = msg.at(2);
                case 2: val0 = msg.at(1);
                case 1: case 0: break;
            }
            normalEvent(ts, status, val0, val1);
            return;
        }
    }
    for (auto it = std::begin(msg) + 1, et = std::end(msg); it != et; ++it) {
        auto datum = *it;
        if (int(datum) == 0xf7) {
            sysExEvent(ts, m_sysEx);
            m_sysEx.clear();
            m_inSysEx = false;
            return;
        } else {
            m_sysEx.append(datum);
        }
    }
}

QString MidiController::deviceName() {
    return m_deviceName;
}

void MidiController::setDeviceName(QString deviceName) {
    if (m_deviceName != deviceName) {
        m_deviceName = deviceName;
        reloadDevice();
        emit deviceNameChanged(m_deviceName);
    }
}

bool MidiController::connected() {
    return m_connected;
}
