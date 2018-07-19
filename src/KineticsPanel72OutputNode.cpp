#include "KineticsPanel72OutputNode.h"
#include <iostream>
#include <algorithm>
#include <string.h>

KineticsPanel72OutputNode::KineticsPanel72OutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(new KineticsPanel72OutputNodePrivate(context, chainSize), context, chainSize, 20) {
    connect(this, &SelfTimedReadBackOutputNode::frame, this, &KineticsPanel72OutputNode::onFrame, Qt::DirectConnection);
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
    d()->outPacket->fill(0, 681);
    char const *packetHeader = "\x04\x01\xdc\x4a\x01\x00\x08\x01";
    d()->outPacket->replace(0, 8, packetHeader,8);
    d()->outPacket->replace(17, 1, "\xd1");
    d()->outPacket->replace(21, 1, "\x02");
    d()->outPacket->replace(680, 1, "\xbf");
    start();
}

KineticsPanel72OutputNode::KineticsPanel72OutputNode(const KineticsPanel72OutputNode &other)
: SelfTimedReadBackOutputNode(other) {
}

KineticsPanel72OutputNode *KineticsPanel72OutputNode::clone() const {
    return new KineticsPanel72OutputNode(*this);
}

QSharedPointer<KineticsPanel72OutputNodePrivate> KineticsPanel72OutputNode::d() const {
    return d_ptr.staticCast<KineticsPanel72OutputNodePrivate>();
}

void KineticsPanel72OutputNode::onFrame(QSize size, QByteArray frame) {
    // there are two controllers for each panel
    int numPanels = (d()->sEnd - d()->sStart + 1) *2;
    for(int pIdx = 0; pIdx < numPanels; pIdx++){
        // set the universe in the dmx protocol
        (*d()->outPacket)[16] = pIdx+1;

        int panelNum = d()->sStart*2 + pIdx;

        // compute the x range, on even controllers the range decrements and on
        // odd it increments
        int startI = panelNum%2 ==0 ? (panelNum+1)*6-1 : panelNum*6;
        startI %= d()->fWidth;
        int endI = panelNum%2 ==0 ? startI-6 : startI+6;
        int deltaI = panelNum%2 ==0 ? -1 : 1;
        // the y range always increments, but due to the order of the panels
        // each panel starts at a decrementing index
        int startJ = d()->fHeight - 1 - (((panelNum*6)/d()->fWidth+1)*12 -1);
        int endJ = d()->fHeight - 1 - ((panelNum*6)/d()->fWidth *12);

        int counter = 0;
        for(int frameI = startI; abs(frameI-endI) > 0; frameI+=deltaI) {
            for(int frameJ = startJ; frameJ <= endJ; frameJ++) {
                // iterate through the packets to be sent to the lights and get
                // the appropriate bit from the image frame
                int index = frameJ*d()->fWidth + frameI;
                (*d()->outPacket)[24 + counter*3] = frame.at(index*4);
                (*d()->outPacket)[24 + counter*3+1] = frame.at(index*4+1);
                (*d()->outPacket)[24 + counter*3+2] = frame.at(index*4+2);
                counter++;
            }
        }

        //send bytes to the device
        d()->udpSocket->write(*d()->outPacket);
    }
}

QString KineticsPanel72OutputNode::typeName() {
    return "KineticsPanel72OutputNode";
}

QJsonObject KineticsPanel72OutputNode::parameters() {
    QMutexLocker locker(&d()->m_stateLock);
    return *d()->m_parameters;
}

QJsonObject KineticsPanel72OutputNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    QJsonObject params = parameters();
    for (QString key : params.keys()) {
      o.insert(key, params.value(key));
    }
    return o;
}

void KineticsPanel72OutputNode::initDataSocket(QJsonObject obj) {
    QHostAddress host = *new QHostAddress();
    host.setAddress(obj.value("host").toString());
    d()->udpSocket->connectToHost(host, 6038);
    std::string sectionStart = obj.value("sectionStart").toString().toUtf8().constData();
    sectionStart = sectionStart.compare("") == 0 ? "1" : sectionStart;
    std::string sectionEnd = obj.value("sectionEnd").toString().toUtf8().constData();
    sectionEnd = sectionEnd.compare("") == 0  ? "1" : sectionEnd;
    std::string fullWidth = obj.value("fullWidth").toString().toUtf8().constData();
    fullWidth = fullWidth.compare("") == 0 ? "1" : fullWidth;
    std::string fullHeight = obj.value("fullHeight").toString().toUtf8().constData();
    fullHeight = fullHeight.compare("") == 0 ? "1" : fullHeight;
    d()->sStart = std::stoi(sectionStart)-1;
    d()->sEnd = std::stoi(sectionEnd)-1;
    d()->fWidth = std::stoi(fullWidth)*12;
    d()->fHeight = std::stoi(fullHeight)*12;
}

VideoNode *KineticsPanel72OutputNode::deserialize(Context *context, QJsonObject obj) {
    std::string fullWidth = obj.value("fullWidth").toString().toUtf8().constData();
    fullWidth = fullWidth.compare("") == 0 ? "1" : fullWidth;
    std::string fullHeight = obj.value("fullHeight").toString().toUtf8().constData();
    fullHeight = fullHeight.compare("") == 0 ? "1" : fullHeight;
    int fWidth = std::stoi(fullWidth)*12;
    int fHeight = std::stoi(fullHeight)*12;

    KineticsPanel72OutputNode *e = new KineticsPanel72OutputNode(context, QSize(fWidth, fHeight));
    e->initDataSocket(obj);
    e->d()->m_parameters = new QJsonObject({
      {"host", obj.value("host").toString()},
      {"fullWidth", obj.value("fullWidth").toString()},
      {"fullHeight", obj.value("fullHeight").toString()},
      {"sectionStart", obj.value("sectionStart").toString()},
      {"sectionEnd", obj.value("sectionEnd").toString()}
    });
    return e;
}

bool KineticsPanel72OutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *KineticsPanel72OutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> KineticsPanel72OutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("Color Kinetics Panel72", "KineticsPanel72OutputInstantiator.qml");
    return m;
}

KineticsPanel72OutputNodePrivate::KineticsPanel72OutputNodePrivate(Context *context, QSize chainSize)
: SelfTimedReadBackOutputNodePrivate(context, chainSize)
{
    outPacket = new QByteArray();
    udpSocket = new QUdpSocket();
}
