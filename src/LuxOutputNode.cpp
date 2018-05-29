#include "LuxOutputNode.h"
#include "liblux/lux.h"
#include <QMutexLocker>

LuxOutputNode::LuxOutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(context, chainSize, 10) {

    connect(this, &SelfTimedReadBackOutputNode::frame, this, &LuxOutputNode::onFrame, Qt::DirectConnection);

    reload();
    start();
}

LuxOutputNode::LuxOutputNode(const LuxOutputNode &other)
    : SelfTimedReadBackOutputNode(other) {
}

int LuxOutputNode::fd() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_fd;
}

void LuxOutputNode::setFd(int fd) {
    QMutexLocker locker(&d()->m_stateLock);
    d()->m_fd = fd;
}

void LuxOutputNode::reload() {
    qDebug() << "Attempting lux open";
    const char lux_uri[] = "serial:///dev/ttyACM0";
    Q_UNUSED(lux_uri);
    int lux_fd = lux_uri_open(lux_uri);
    if (lux_fd < 0) {
        qDebug() << QString("Could not open lux uri \"%1\"").arg(lux_uri);
        emit fatal(QString("Could not open lux uri \"%1\"").arg(lux_uri));
        return;
    }
    setFd(lux_fd);
    qDebug() << "Lux opened.";
}

void LuxOutputNode::onFrame(QSize size, QByteArray frame) {
    int lux_fd = fd();
    if (lux_fd < 0) return;

    // Compute the frame
    QByteArray strip(50, 0);
    for (int i=0; i<50; i++) {
        double x = 0.5;
        double y = (double)i / 49.;

        int col = qMax(qMin(qRound(x * size.width()), size.width() - 1), 0);
        int row = qMax(qMin(qRound(y * size.height()), size.height() - 1), 0);
        int pixel = 4 * (row * size.height() + col);
        strip[3 * i + 0] = frame[pixel + 1];
        strip[3 * i + 1] = frame[pixel + 0];
        strip[3 * i + 2] = frame[pixel + 2];
    }

    // Now we have the frame, so send it
    struct lux_packet packet;
    packet.destination = 1; // send to id 1
    packet.command = LUX_CMD_FRAME;
    packet.index = 0;
    packet.payload_length = strip.size();
    uint8_t * payload = packet.payload;
    for (auto c : strip) *payload++ = c;
    int rc = lux_write(lux_fd, &packet, (enum lux_flags) 0);
    if (rc != 0) {
        qDebug() << "lux_strip_frame returned" << rc;
    }
}

QString LuxOutputNode::typeName() {
    return "LuxOutputNode";
}

VideoNode *LuxOutputNode::deserialize(Context *context, QJsonObject obj) {
    LuxOutputNode *e = new LuxOutputNode(context, QSize(100, 100));
    return e;
}

bool LuxOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *LuxOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
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
    , m_fd(-1)
{
}

LuxOutputNodePrivate::~LuxOutputNodePrivate() {
    // TODO RAII
    if (m_fd >= 0) {
        lux_close(m_fd);
        qDebug() << "Lux closed.";
        m_fd = -1;
    }
}
