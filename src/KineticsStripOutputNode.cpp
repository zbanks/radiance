#include "KineticsStripOutputNode.h"
#include <iostream>
#include <algorithm>
#include <string.h>

KineticsStripOutputNode::KineticsStripOutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(new KineticsStripOutputNodePrivate(context, chainSize), context, chainSize, 10) {
    connect(this, &SelfTimedReadBackOutputNode::frame, this, &KineticsStripOutputNode::onFrame, Qt::DirectConnection);

    /*data packet header (
    ("magic", "I", 0x0401dc4a),
    ("version", "H", 0x0100),
    ("type", "H", 0x0101),
    ("sequence", "I", 0x00000000),
    ("port", "B", 0x00),
    ("padding", "B", 0x00),
    ("flags", "H", 0x0000),
    ("timer", "I", 0xffffffff),
    ("universe", "B", 0x00),)*/

    // 21 bytes for the header; 3 bytes (RGB) for each pixel
    //magic, version, type, sequence, port, padding, flags, timer, universe
    unsigned char packetHeader[21] = {0x04, 0x01, 0xdc, 0x4a, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00};
    for(int i =0; i < 21; i++) {
        d()->outPacket->append(packetHeader[i]);
    }
    start();
}

KineticsStripOutputNode::KineticsStripOutputNode(const KineticsStripOutputNode &other)
: SelfTimedReadBackOutputNode(other) {
}

KineticsStripOutputNode *KineticsStripOutputNode::clone() const {
    return new KineticsStripOutputNode(*this);
}

QSharedPointer<KineticsStripOutputNodePrivate> KineticsStripOutputNode::d() const {
    return d_ptr.staticCast<KineticsStripOutputNodePrivate>();
}

void KineticsStripOutputNode::onFrame(QSize size, QByteArray frame) {
    // outPacketIdx is the packet size
    d()->outPacket->truncate(21);

    //filter the rgba values from radiance into rgb for kinetics

    d()->outPacket->append(d()->dmxPort, 0);

    for (int i = 0; i < frame.size(); i+=4) {
        d()->outPacket->append(frame.at(i));
        d()->outPacket->append(frame.at(i+1));
        d()->outPacket->append(frame.at(i+2));
    }
    //send bytes to the device
    d()->udpSocket->writeDatagram(*d()->outPacket, *d()->host, 6038);
}

QString KineticsStripOutputNode::typeName() {
    return "KineticsStripOutputNode";
}

QJsonObject KineticsStripOutputNode::parameters() {
    QMutexLocker locker(&d()->m_stateLock);
    return *d()->m_parameters;
}

QJsonObject KineticsStripOutputNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    QJsonObject params = parameters();
    for (QString key : params.keys()) {
      o.insert(key, params.value(key));
    }
    return o;
}

void KineticsStripOutputNode::initDataSocket(QJsonObject obj) {
    d()->host->setAddress(obj.value("host").toString());
    std::string sdmxPort = obj.value("dmxPort").toString().toUtf8().constData();
    sdmxPort = sdmxPort.compare("") == 0 ? "0" : sdmxPort;
    d()->dmxPort = std::stoi(sdmxPort);
}

VideoNode *KineticsStripOutputNode::deserialize(Context *context, QJsonObject obj) {
    std::string swidth = obj.value("width").toString().toUtf8().constData();
    swidth = swidth.compare("") == 0 ? "1" : swidth;
    std::string sheight = obj.value("height").toString().toUtf8().constData();
    sheight = sheight.compare("") == 0 ? "1" : sheight;

    int width = std::stoi(swidth);
    int height = std::stoi(sheight);

    KineticsStripOutputNode *e = new KineticsStripOutputNode(context, QSize(width, height));
    e->initDataSocket(obj);
    e->d()->m_parameters = new QJsonObject({
      {"width", obj.value("width").toString()},
      {"height", obj.value("height").toString()},
      {"host", obj.value("host").toString()},
      {"dmxPort", obj.value("port").toString()}
    });
    return e;
}

bool KineticsStripOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *KineticsStripOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> KineticsStripOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("Color Kinetics Strip", "KineticsStripOutputInstantiator.qml");
    return m;
}

KineticsStripOutputNodePrivate::KineticsStripOutputNodePrivate(Context *context, QSize chainSize)
: SelfTimedReadBackOutputNodePrivate(context, chainSize)
{
    outPacket = new QByteArray();
    host = new QHostAddress();
    udpSocket = new QUdpSocket();

    //state that gets (de)serialized on restart
    QJsonObject m_parameters;
}
