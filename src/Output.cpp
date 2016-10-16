#include "Output.h"
#include "Lux.h"

// OutputBus

OutputManager::OutputManager() {

}

QList<QObject *> OutputManager::buses() {
    return m_buses;
}

void OutputManager::createBus(QString uri) {
    LuxBus * luxbus = new LuxBus();
    luxbus->setUri(uri);
    m_buses.append(luxbus);
    emit busesChanged(m_buses);
}

// OutputBus

OutputBus::OutputBus() :
    m_state(Disconnected) {
}

OutputBus::OutputBus(QString uri) :
    m_state(Disconnected),
    m_uri(uri) {
}

OutputBus::~OutputBus() {
    for (std::vector<OutputDevice *>::iterator it = m_devices.begin(); it != m_devices.end(); it++) {
        delete *it;
    }
}

QString OutputBus::uri() {
    return m_uri;
}

OutputBus::State OutputBus::state() {
    return m_state;
}

void OutputBus::setUri(QString uri) {
    m_uri = uri;
    m_state = Disconnected;
    emit stateChanged(m_state);
    emit uriChanged(m_uri);
}
