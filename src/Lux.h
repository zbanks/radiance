#pragma once

#include <QPolygonF>
#include <QQuickItem>
#include <QSettings>
#include <QThread>
#include <QList>
#include <string>
#include <vector>

class LuxBus;

class LuxDevice : public QQuickItem {
    Q_OBJECT
    Q_ENUMS(State);
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QPolygonF polygon READ polygon WRITE setPolygon NOTIFY polygonChanged)
    Q_PROPERTY(quint32 luxId READ luxId WRITE setLuxId NOTIFY luxIdChanged)
    Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
public:
    enum class State { Disconnected, Connected, Blind, Error };

    LuxDevice(QQuickItem * parent = nullptr);
    ~LuxDevice() = default;

    void loadSettings(QSettings * settings);
    void saveSettings(QSettings * settings);

    State state();
    QString name();
    void setName(QString name);
    QColor color();
    void setColor(QColor color);
    QPolygonF polygon();
    void setPolygon(QPolygonF polygon);
    quint32 luxId();
    void setLuxId(quint32 luxId);
    int length();
    void setLength(int length);

    void frame();

public slots:
    void refresh();

signals:
    void stateChanged(State state);
    void nameChanged(QString name);
    void colorChanged(QColor color);
    void polygonChanged(QPolygonF polygon);
    void luxIdChanged(quint32 luxId);
    void lengthChanged(int length);

protected:
    int m_length;

    State m_state;
    QString m_name;
    QColor m_color;
    QPolygonF m_polygon;
    LuxBus * m_bus;
    quint32 m_id;
};


class LuxBus : public QQuickItem {
    Q_OBJECT
    Q_ENUMS(State);
    Q_PROPERTY(QString uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    enum class State { Disconnected, Connected, Error };

    LuxBus(QQuickItem * parent = nullptr);
    ~LuxBus();

    void loadSettings(QSettings * settings);
    void saveSettings(QSettings * settings);

    QString uri();
    void setUri(QString uri);
    State state();

    void beginFrame();
    void endFrame();
    void detectDevices(QList<LuxDevice *> device_hints);

public Q_SLOTS:
    void refresh();

signals:
    void stateChanged(State state);
    void uriChanged(QString uri);

protected:
    State m_state;
    QString m_uri;
    QList<LuxDevice *> m_devices;
    int m_fd;
};
