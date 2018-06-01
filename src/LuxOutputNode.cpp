#include "LuxOutputNode.h"
#include "Lux.h"
#include "liblux/lux.h"
#include <QMutexLocker>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

LuxOutputNode::LuxOutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(context, chainSize, 10) {

    connect(this, &SelfTimedReadBackOutputNode::frame, this, &LuxOutputNode::onFrame, Qt::DirectConnection);

    reload();
    start();
}

LuxOutputNode::LuxOutputNode(const LuxOutputNode &other)
    : SelfTimedReadBackOutputNode(other) {
}

void LuxOutputNode::setDevices(QList<QSharedPointer<LuxDevice>> devices) {
    // XXX Is this really sufficient?
    QMutexLocker locker(&d()->m_stateLock);
    d()->m_devices = devices;
}

QList<QSharedPointer<LuxDevice>> LuxOutputNode::devices() {
    // XXX Is this really sufficient?
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_devices;
}

void LuxOutputNode::setBuses(QList<QSharedPointer<LuxBus>> buses) {
    // XXX Is this really sufficient?
    QMutexLocker locker(&d()->m_stateLock);
    d()->m_buses = buses;
}

QList<QSharedPointer<LuxBus>> LuxOutputNode::buses() {
    // XXX Is this really sufficient?
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_buses;
}

void LuxOutputNode::reload() {
    qDebug() << "Attempting lux open";
    for (auto bus : buses()) {
        bus->refresh();
        bus->detectDevices(devices());
    }
    qDebug() << "Lux opened.";
}

void LuxOutputNode::onFrame(QSize size, QByteArray frame) {
    for (auto bus : buses()) {
        bus->frame(size, frame);
    }
}

QString LuxOutputNode::typeName() {
    return "LuxOutputNode";
}

QPolygonF deserializePolygon(QJsonArray arr) {
    QPolygonF polygon;
    for (auto _xy : arr) {
        QJsonArray xy = _xy.toArray();
        polygon << QPointF(xy[0].toDouble(), xy[1].toDouble());
    }
    return polygon;
}

QJsonArray serializePolygon(QPolygonF polygon) {
    QJsonArray arr;
    for (auto xy : polygon) {
        QJsonArray jsonPoint;
        jsonPoint << xy.x() << xy.y();
        arr << jsonPoint;
    }
    return arr;
}

QJsonObject LuxOutputNode::serialize() {
    QJsonObject o = VideoNode::serialize();

    QJsonArray jsonBuses;
    for (auto bus : buses()) {
        jsonBuses << bus->uri();
    }
    o.insert("buses", jsonBuses);

    QJsonArray jsonDevices;
    for (auto device : devices()) {
        QJsonObject jsonDevice;
        jsonDevice.insert("name", device->name());
        jsonDevice.insert("lux_id", (int) device->luxId());
        jsonDevice.insert("length", device->length());
        jsonDevice.insert("polygon", serializePolygon(device->polygon()));
        jsonDevices << jsonDevice;
    }
    o.insert("devices", jsonDevices);
    return o;
}

VideoNode *LuxOutputNode::deserialize(Context *context, QJsonObject obj) {
    // TODO: error handling
    QList<QSharedPointer<LuxBus>> busList;
    QJsonArray jsonBuses = obj["buses"].toArray();
    for (auto jsonBus : jsonBuses) {
        QString busUri = jsonBus.toString();
        QSharedPointer<LuxBus> bus = QSharedPointer<LuxBus>::create();
        bus->setUri(busUri);
        busList << bus;
    }

    QList<QSharedPointer<LuxDevice>> deviceList;
    QJsonArray jsonDevices = obj["devices"].toArray();
    for (auto _jsonDevice : jsonDevices) {
        QJsonObject jsonDevice = _jsonDevice.toObject();
        QSharedPointer<LuxDevice> device = QSharedPointer<LuxDevice>::create();
        device->setType(LuxDevice::Type::Strip);
        device->setName(jsonDevice["name"].toString());
        device->setLuxId(jsonDevice["lux_id"].toInt());
        device->setLength(jsonDevice["length"].toInt());
        device->setPolygon(deserializePolygon(jsonDevice["polygon"].toArray()));
        deviceList << device;
    }

    LuxOutputNode *e = new LuxOutputNode(context, QSize(100, 100));
    e->setDevices(deviceList);
    e->setBuses(busList);
    e->reload();


    return e;
}

bool LuxOutputNode::canCreateFromFile(QString filename) {
    return filename.endsWith(".lux.json");
}

VideoNode *LuxOutputNode::fromFile(Context *context, QString filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open file for reading:" << filename;
        return nullptr;
    }
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return deserialize(context, doc.object());
}

QMap<QString, QString> LuxOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("LuxOutput", "LuxOutputInstantiator.qml");
    return m;
}

LuxOutputNode *LuxOutputNode::clone() const {
    return new LuxOutputNode(*this);
}

QSharedPointer<LuxOutputNodePrivate> LuxOutputNode::d() const {
    return d_ptr.staticCast<LuxOutputNodePrivate>();
}

LuxOutputNode::LuxOutputNode(QSharedPointer<LuxOutputNodePrivate> other_ptr)
    : SelfTimedReadBackOutputNode(other_ptr.staticCast<SelfTimedReadBackOutputNodePrivate>())
{
}

LuxOutputNodePrivate::LuxOutputNodePrivate(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNodePrivate(context, chainSize)
    , m_devices()
    , m_buses()
{
}

LuxOutputNodePrivate::~LuxOutputNodePrivate() {
}
