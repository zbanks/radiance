#pragma once

#include <QPolygonF>
#include <QQuickItem>
#include <QSettings>
#include <QThread>
#include <vector>

// what is C++

class OutputManager;
class OutputBus;

class OutputDevice : public QQuickItem {
    Q_OBJECT
    Q_ENUMS(State);
    Q_ENUMS(Type);
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QPolygonF polygon READ polygon WRITE setPolygon NOTIFY polygonChanged)
public:
    enum State { Disconnected, Connected, Blind, Error };
    enum Type { LuxStripDevice };
    //enum Type { LuxStrip, LuxPoint, LuxGrid, Screen, DMXPoint };

    void loadSettings(QSettings * settings);
    void saveSettings(QSettings * settings);

    State state();
    QString name();
    void setName(QString name);
    QColor color();
    void setColor(QColor color);
    QPolygonF polygon();
    void setPolygon(QPolygonF polygon);
    
    virtual void frame() = 0;

public slots:
    virtual void refresh() = 0;

signals:
    void stateChanged(State state);
    void nameChanged(QString name);
    void colorChanged(QColor color);
    void polygonChanged(QPolygonF polygon);

protected:
    OutputManager * m_manager;
    State m_state;
    QString m_name;
    QColor m_color;
    QPolygonF m_polygon;
};

class OutputBus : public QQuickItem {
    Q_OBJECT
    Q_ENUMS(State);
    Q_ENUMS(Type);
    Q_PROPERTY(QString uri READ uri NOTIFY uriChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    enum State { Disconnected, Connected, Error };
    enum Type { LuxBus };

    QString uri();
    State state();

    void loadSettings(QSettings * settings);
    void saveSettings(QSettings * settings);

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void detectDevices(QVector<OutputDevice *> device_hints) = 0;

public slots:
    virtual void refresh() = 0;

signals:
    void stateChanged(State state);
    void uriChanged(QString uri);

protected:
    OutputManager * m_manager;
    State m_state;
    QString m_uri;
    QVector<OutputDevice *> m_devices;
};

// OutputManager singleton
class OutputManager : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QList<QObject *> buses READ buses NOTIFY busesChanged)
    Q_PROPERTY(QList<QObject *> devices READ devices NOTIFY devicesChanged)

public:
    OutputManager(QSettings * settings);
    OutputManager(QObject * p = nullptr);
    ~OutputManager();

    void saveSettings(QSettings * settings = nullptr);

    QList<QObject *> buses();
    Q_INVOKABLE void createBus(OutputBus::Type type, QString uri);

    QList<QObject *> devices();
    //Q_INVOKABLE void createDevice(OutputDevice::Type type, QString uri);

public slots:
    void refresh();

signals:
    void busesChanged(QList<QObject *>);
    void devicesChanged(QList<QObject *>);

private:
    // Not sure if we want this to be a method or not
    void loadSettings(QSettings * settings = nullptr);

    QList<QObject *> m_buses;
    QList<QObject *> m_devices;
    QSettings * m_settings;
};

