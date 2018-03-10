#pragma once

#include "SelfTimedReadBackOutputNode.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <QHostAddress>
#include <QUdpSocket>

class KineticsPanel72OutputNodePrivate;

class KineticsPanel72OutputNode
    : public SelfTimedReadBackOutputNode {
    Q_OBJECT

public:
    KineticsPanel72OutputNode(Context *context, QSize chainSize);
    KineticsPanel72OutputNode(const KineticsPanel72OutputNode &other);
    KineticsPanel72OutputNode *clone() const override;
    void onFrame(QSize size, QByteArray frame);
    void initDataSocket(QJsonObject obj);

    // These static methods are required for VideoNode creation
    // through the registry

    // A string representation of this VideoNode type
    static QString typeName();

    // Create a VideoNode from a JSON description of one
    // Returns nullptr if the description is invalid
    static VideoNode *deserialize(Context *context, QJsonObject obj);

    // Return true if a VideoNode could be created from
    // the given filename
    // This check should be very quick.
    static bool canCreateFromFile(QString filename);

    // Create a VideoNode from a filename
    // Returns nullptr if a VideoNode cannot be create from the given filename
    static VideoNode *fromFile(Context *context, QString filename);

    // Returns QML filenames that can be loaded
    // to instantiate custom instances of this VideoNode
    static QMap<QString, QString> customInstantiators();
    private:
        QSharedPointer<KineticsPanel72OutputNodePrivate> d() const;
};

class KineticsPanel72OutputNodePrivate : public SelfTimedReadBackOutputNodePrivate {
public:
    KineticsPanel72OutputNodePrivate(Context *context, QSize chainSize);

    // Reusable state for sending data packets
    QUdpSocket *udpSocket;
    QByteArray *outPacket;
    int sStart;
    int sEnd;
    int fWidth;
    int fHeight;
};
