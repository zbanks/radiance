#pragma once

#include <QThread>
#include <QQuickItem>
#include <vector>

// what is C++

class OutputBus;
class OutputDevice;

class OutputManager : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QList<QObject *> buses READ buses NOTIFY busesChanged)

public:
    OutputManager();

    QList<QObject *> buses();
    Q_INVOKABLE void createBus(QString uri);

signals:
    void busesChanged(QList<QObject *>);

private:
    QList<QObject *> m_buses;
};


class OutputDevice : public QQuickItem {
public:
    OutputDevice(OutputBus * bus);
    /*
    virtual ~OutputDevice() {};
    virtual void sendFrame() = 0;
    virtual void syncFrame() = 0;
    */

private:
    OutputBus * m_bus;
};

class OutputBus : public QQuickItem {
    Q_OBJECT
    Q_ENUMS(State);
    Q_PROPERTY(QString uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    enum State { Disconnected, Connected, Error };

    OutputBus();
    OutputBus(QString uri);
    ~OutputBus();

    QString uri();
    State state();
    virtual void setUri(QString uri);

public Q_SLOTS:
    virtual void refresh() = 0;
    virtual void sendFrames() = 0;
    virtual void syncFrames() = 0;

signals:
    void uriChanged(QString uri);
    void stateChanged(State state);

protected:
    State m_state;
    QString m_uri;
    std::vector<OutputDevice *> m_devices;
};

