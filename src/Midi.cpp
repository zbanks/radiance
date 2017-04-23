#include "Midi.h"

static const int MAX_POLL_SIZE = 2048;

MidiDevice::MidiDevice(QQuickItem * parent) :
    QQuickItem(parent),
    m_deviceIndex(-1),
    m_midiin()
{
    reload();
    connect(this, &MidiDevice::normalEvent, this, &MidiDevice::onNormalEvent,Qt::QueuedConnection);
    connect(this, &MidiDevice::realtimeEvent, this, &MidiDevice::onRealtimeEvent,Qt::QueuedConnection);
}


MidiDevice::~MidiDevice()
{
    m_midiin.closePort();
}
void MidiDevice::onRealtimeEvent(double ts, int status)
{
    switch(status) {
        case 0xf8: emit rtClock(ts);break;
        case 0xfa: emit rtStart(ts);break;
        case 0xfb: emit rtContinue(ts);break;
        case 0xfc: emit rtStop(ts); break;
        case 0xfe: emit rtActiveSensing(ts);break;
        case 0xff: emit rtReset(ts);break;
        default:break;
    }
}
void MidiDevice::onNormalEvent(double ts, int status, int val0, int val1)
{
    auto ch = status&0xf;
    switch((status>>4)&7) {
        case 0: noteOn(ts,ch, val0, val1);break;
        case 1: noteOff(ts,ch, val0, val1);break;
        case 2: noteAftertouch(ts,ch,val0,val1);break;
        case 3: controlChange(ts, ch, val0, val1);break;
        case 4: programChange(ts, ch, val0);break;
        case 5: channelAftertouch(ts, ch, val0);break;
        case 6: pitchBend(ts, ch, ((val0&0x7f)|((val1&0x7f)<<7)) - 0x2000);break;
        default: break;
    }
}
QStringList MidiDevice::deviceList()
{
    return m_deviceList;
}

int MidiDevice::deviceIndex() const
{
    return m_deviceIndex;
}

void MidiDevice::setDeviceIndex(int idx)
{
    if(deviceIndex() == idx)
        return;
    if(m_midiin.isPortOpen()) {
        try {
        // TODO: Catch exceptions or something?
            m_midiin.closePort();
        } catch(... ) {
        }
    }
    try {
        if (idx < 0 || idx >= m_midiin.getPortCount()) {
            idx = -1;
        } else {
            auto trampoline = [](double ts, std::vector<unsigned char> *msg, void *opaque) {
                if(opaque && msg)
                    static_cast<MidiDevice*>(opaque)->callback(ts, *msg);
            };
            m_midiin.cancelCallback();
            m_midiin.setCallback(static_cast<RtMidiIn::RtMidiCallback>(trampoline), static_cast<void*>(this));
            m_midiin.openPort(idx, "Radiance Input");
        }
        m_deviceIndex = idx;
        emit deviceIndexChanged(m_deviceIndex);
    }catch(...) {
    }
}

void MidiDevice::reload() {
    // TODO: Catch exceptions or something?
    try {
        auto dl = QStringList{};
        unsigned int count = m_midiin.getPortCount();
        for (unsigned int i = 0; i < count; i++)
            dl << QString::fromStdString(m_midiin.getPortName(i));
        m_deviceList.swap(dl);
        if(m_deviceList != dl)
            emit deviceListChanged(m_deviceList);
    }catch(...) {
    }
    auto di = -1;
    using std::swap;
    swap(di,m_deviceIndex);
    setDeviceIndex(di);
}

void MidiDevice::callback(double ts, std::vector<uint8_t>&msg)
{
    m_lastTime += ts;
    ts = m_lastTime;
    if(msg.empty())
        return;
    auto status = int(msg.front());
    if((status&0xF8) == 0xF8) {
        realtimeEvent(ts,status);
        return;
    }
    if(!m_inSysEx) {
        if(status == 0xF0) {
            status = 0;
            m_inSysEx = true;
        }else{
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
    for(auto it = std::begin(msg) + 1, et = std::end(msg); it != et; ++it) {
        auto datum = *it;
        if(int(datum) == 0xf7) {
            sysExEvent(ts, m_sysEx);
            m_sysEx.clear();
            m_inSysEx = false;
            return;
        }else{
            m_sysEx.append(datum);
        }
    }
}
