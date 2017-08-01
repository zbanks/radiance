#include "Lux.h"
#include "liblux/lux.h"
#include <unistd.h>
#include <math.h>
#include "RenderContextOld.h"

// Lux helper functions
static int lux_strip_get_length (int fd, uint32_t lux_id, int flags) {
    // TODO: replace with get_descriptor
    struct lux_packet packet;
    packet.destination = lux_id;
    packet.command = LUX_CMD_GET_LENGTH;
    packet.index = 0;
    packet.payload_length = 0;

    struct lux_packet response;
    int rc = lux_command(fd, &packet, &response, (enum lux_flags) flags);
    if (rc < 0 || response.payload_length < 2) {
        qInfo("No/invalid response to length query on %#08x", lux_id);
        return -1;
    }

    uint16_t length;
    memcpy(&length, response.payload, sizeof length);

    qInfo("Found strip on %#08x with length %d", lux_id, length);

    return length;
}

static int lux_strip_frame (int fd, uint32_t lux_id, QVector<QColor> data) {
    struct lux_packet packet;
    packet.destination = lux_id;
    packet.command = LUX_CMD_FRAME;
    packet.index = 0;
    packet.payload_length = data.size() * 3;
    uint8_t * payload = packet.payload;
    for (QColor c : data) {
        *payload++ = c.red();
        *payload++ = c.green();
        *payload++ = c.blue();
    }
    return lux_write(fd, &packet, (enum lux_flags) 0);
}

// LuxDevice
LuxDevice::LuxDevice(QQuickItem * parent) :
    QQuickItem(parent),
    m_state(State::Disconnected),
    m_type(Type::Strip),
    m_length(0),
    m_polygon(QVector<QPointF>{QPointF(0.,0.), QPointF(1.,1.)}), // kill once we actually have polys
    m_color(QColor(255, 255, 0)),
    m_id(0) {
    arrangePixels();
}
LuxDevice::State LuxDevice::state() {
    return m_state;
}
LuxDevice::Type LuxDevice::type() {
    return m_type;
}
void LuxDevice::setType(LuxDevice::Type type) {
    m_type = type;
    arrangePixels();
    emit typeChanged(m_type);
}
QString LuxDevice::name() {
    return m_name;
}
void LuxDevice::setName(QString name) {
    m_name = name;
    emit nameChanged(m_name);
}
QColor LuxDevice::color() {
    return m_color;
}
void LuxDevice::setColor(QColor color) {
    m_color = color;
    emit colorChanged(m_color);
}
QPolygonF LuxDevice::polygon() {
    return m_polygon;
}
void LuxDevice::setPolygon(QPolygonF polygon) {
    m_polygon = polygon;
    arrangePixels();
    emit polygonChanged(m_polygon);
}
quint32 LuxDevice::luxId() {
    return m_id;
}
void LuxDevice::setLuxId(quint32 id) {
    if (id != m_id) {
        m_id = id;
        m_state = State::Disconnected;
        emit luxIdChanged(m_id);
        emit stateChanged(m_state);
    }
}
int LuxDevice::length() {
    return m_length;
}
void LuxDevice::setLength(int length) {
    m_length = length;
    emit lengthChanged(m_length);
}
void LuxDevice::loadSettings(QSettings * settings) {
    setName(settings->value("name").toString());
    setColor(settings->value("color").value<QColor>());
    setPolygon(settings->value("polygon").value<QPolygonF>());
    setLength(settings->value("length").toInt());
    setLuxId(settings->value("luxId").value<quint32>());
}
void LuxDevice::saveSettings(QSettings * settings) {
    settings->setValue("name", m_name);
    settings->setValue("color", m_color);
    settings->setValue("polygon", m_polygon);
    settings->setValue("length", m_length);
    settings->setValue("luxId", m_id);
}

void LuxDevice::arrangePixels() {
    m_pixels.clear();
    if (m_polygon.size() == 0) {
        m_pixels.fill(QPointF(0., 0.), m_length);
        return;
    }
    if (m_polygon.size() == 1) {
        m_pixels.fill(m_polygon.at(0), m_length);
        return;
    }
    // This only does Type::Strip for now
    qreal scale_sum = 0.;
    for (size_t i = 1; i < m_polygon.size(); i++) {
        QPointF delta = m_polygon.at(i-1) - m_polygon.at(i);
        scale_sum += hypot(delta.x(), delta.y());
    }
    qreal scale_per_pixel = scale_sum / (qreal) m_length;
    qreal cumulative_scale = 0.;
    size_t pixel_idx = 0;
    for (size_t i = 1; i < m_polygon.size(); i++) {
        QPointF delta = m_polygon.at(i-1) - m_polygon.at(i);
        qreal scale = hypot(delta.x(), delta.y());
        while (pixel_idx * scale_per_pixel <= cumulative_scale + scale) {
            if (pixel_idx >= m_length) break;
            qreal alpha = (pixel_idx * scale_per_pixel - cumulative_scale) / scale;
            m_pixels[pixel_idx++] = alpha * m_polygon.at(i) + (1.0 - alpha) * m_polygon.at(i-1);
        }
        cumulative_scale += scale;
    }
}
void LuxDevice::setBus(LuxBus * bus, bool blind) {
    if (blind)
        m_state = State::Blind;
    else
        m_state = State::Connected;
    m_bus = bus;
    emit stateChanged(m_state);
}

QVector<QColor> LuxDevice::frame() {
    if (m_videoNode == nullptr)
        return QVector<QColor>();
    return m_videoNode->pixels(m_videoNode->context()->outputFboIndex(), m_pixels); // TODO put this back
}
void LuxDevice::refresh() {
}

//

// TODO: Use QtSerialPort & QtUdpSocket instead of file descriptors
// to make lux platform-indepdendent
LuxBus::LuxBus(QQuickItem * parent) :
    QQuickItem(parent),
    m_state(State::Disconnected),
    m_fd(-1) {
}

LuxBus::~LuxBus() {
    if (m_fd > 0)
        lux_close(m_fd);
}

QString LuxBus::uri() {
    return m_uri;
}
LuxBus::State LuxBus::state() {
    return m_state;
}
void LuxBus::setUri(QString uri) {
    m_uri = uri;

    if (m_fd >= 0) {
        lux_close(m_fd);
        m_fd = -1;
        m_state = State::Disconnected;
        emit stateChanged(m_state);
    }

    if (!m_uri.isEmpty()) {
        m_fd = lux_uri_open(m_uri.toStdString().c_str());
        if (m_fd < 0)
            m_state = State::Error;
        else
            m_state = State::Connected;
        emit stateChanged(m_state);
    }
    emit uriChanged(m_uri);
}

void LuxBus::loadSettings(QSettings * settings) {
    setUri(settings->value("uri").toString());
}
void LuxBus::saveSettings(QSettings * settings) {
    settings->setValue("uri", m_uri);
}

void LuxBus::refresh() {
    auto it = m_devices.begin();
    while (it != m_devices.end()) {
        LuxDevice * dev = *it;
        if (dev->state() == LuxDevice::State::Disconnected ||
            dev->state() == LuxDevice::State::Error) {
            dev->setBus(nullptr);
            it = m_devices.erase(it);
        } else {
            it++;
        }
    }
}

void LuxBus::frame() {
    if (m_state != State::Connected)
        return;

    for (auto dev : m_devices) {
        int rc = lux_strip_frame(m_fd, dev->luxId(), dev->frame());
        if (rc != 0) {
            m_state = State::Error;
            emit stateChanged(m_state);
        }
    }
}

void LuxBus::detectDevices(QList<LuxDevice *> device_hints) {
    if (m_state != State::Connected)
        return;
    for (auto dev : device_hints) {
        if (dev->state() == LuxDevice::State::Connected ||
            dev->state() == LuxDevice::State::Blind) 
            continue;
        int length = lux_strip_get_length(m_fd, dev->luxId(), 0);
        if (length < 0)
            continue;

        dev->setLength(length);
        dev->setBus(this);
    }
}
