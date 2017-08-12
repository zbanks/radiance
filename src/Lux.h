#pragma once

#include <QPolygonF>
#include <QQuickItem>
#include <QSettings>
#include <QThread>
#include <QList>
#include <string>
#include <vector>
#include "VideoNode.h"

class LuxBus;

class LuxDevice : public QQuickItem {
    Q_OBJECT
    Q_ENUMS(State);
    Q_ENUMS(Type);
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QPolygonF polygon READ polygon WRITE setPolygon NOTIFY polygonChanged)
    Q_PROPERTY(quint32 luxId READ luxId WRITE setLuxId NOTIFY luxIdChanged)
    Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
public:
    enum class State { Disconnected, Connected, Blind, Error };
    enum class Type { Spot, Strip, Grid };

    LuxDevice(QQuickItem * parent = nullptr);
    ~LuxDevice() = default;

    void loadSettings(QSettings * settings);
    void saveSettings(QSettings * settings);

    State state();
    Type type();
    void setType(Type type);
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

    VideoNode * m_videoNode;
    QVector<QColor> frame();

public slots:
    void refresh();
    void setBus(LuxBus * bus, bool blind = false);

signals:
    void stateChanged(State state);
    void typeChanged(Type type);
    void nameChanged(QString name);
    void colorChanged(QColor color);
    void polygonChanged(QPolygonF polygon);
    void luxIdChanged(quint32 luxId);
    void lengthChanged(int length);

private:
    void arrangePixels();

    State m_state;
    Type m_type;
    QString m_name;
    QColor m_color;
    QPolygonF m_polygon;
    quint32 m_id;
    int m_length;

    LuxBus * m_bus;
    QVector<QPointF> m_pixels;
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

    void frame();
    void detectDevices(QList<LuxDevice *> device_hints);

public Q_SLOTS:
    void refresh();

signals:
    void stateChanged(State state);
    void uriChanged(QString uri);

private:
    State m_state;
    QString m_uri;
    QList<LuxDevice *> m_devices;
    int m_fd;
};
