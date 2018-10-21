#include "LightOutputNode.h"
#include "Context.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>

LightOutputNode::LightOutputNode(Context *context, QString url)
    : OutputNode(new LightOutputNodePrivate(context)) {
    attachSignals();
    d()->m_workerContext = new OpenGLWorkerContext(context->threaded());
    d()->m_worker = QSharedPointer<LightOutputNodeOpenGLWorker>(new LightOutputNodeOpenGLWorker(*this), &QObject::deleteLater);
    connect(d()->m_worker.data(), &QObject::destroyed, d()->m_workerContext, &QObject::deleteLater);

    d()->m_chain.moveToWorkerContext(d()->m_workerContext);
    connect(d()->m_worker.data(), &LightOutputNodeOpenGLWorker::sizeChanged, this, &OutputNode::resize);

    if (!url.isEmpty()) setUrl(url);
}

LightOutputNode::LightOutputNode(const LightOutputNode &other)
    : OutputNode(other)
{
    attachSignals();
}

LightOutputNode *LightOutputNode::clone() const {
    return new LightOutputNode(*this);
}

QSharedPointer<LightOutputNodePrivate> LightOutputNode::d() const {
    return d_ptr.staticCast<LightOutputNodePrivate>();
}

LightOutputNode::LightOutputNode(QSharedPointer<LightOutputNodePrivate> other_ptr)
    : OutputNode(other_ptr.staticCast<OutputNodePrivate>())
{
    attachSignals();
}

void LightOutputNode::attachSignals() {
    connect(d().data(), &LightOutputNodePrivate::urlChanged, this, &LightOutputNode::urlChanged);
    connect(d().data(), &LightOutputNodePrivate::nameChanged, this, &LightOutputNode::nameChanged);
}

QString LightOutputNode::typeName() {
    return "LightOutputNode";
}

VideoNode *LightOutputNode::deserialize(Context *context, QJsonObject obj) {
    LightOutputNode *e = new LightOutputNode(context);
    QString url = obj.value("url").toString();
    if (!url.isEmpty()) {
        e->setUrl(url);
    }
    return e;
}

QJsonObject LightOutputNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("url", url());
    return o;
}

bool LightOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *LightOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> LightOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("LightOutput", "LightOutputInstantiator.qml");
    return m;
}

QString LightOutputNode::url() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_url;
}

void LightOutputNode::setUrl(QString value) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        if(d()->m_url == value)
            return;
        d()->m_url = value;
    }

    setNodeState(VideoNode::Loading);
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "initialize");
    Q_ASSERT(result);

    emit d()->urlChanged(value);
}

QString LightOutputNode::name() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_name;
}

void LightOutputNode::setName(QString value) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        if(d()->m_name == value)
            return;
        d()->m_name = value;
    }

    emit d()->nameChanged(value);
}

QMutex *LightOutputNode::bufferLock() {
    return &d()->m_bufferLock;
}

quint32 LightOutputNode::pixelCount() {
    return d()->m_pixelCount;
}

QOpenGLBuffer &LightOutputNode::colorsBuffer() {
    return d()->m_colors;
}

QOpenGLBuffer &LightOutputNode::lookupCoordinatesBuffer() {
    return d()->m_lookupCoordinates;
}

QOpenGLBuffer &LightOutputNode::physicalCoordinatesBuffer() {
    return d()->m_physicalCoordinates;
}

QOpenGLTexture *LightOutputNode::geometry2DTexture() {
    return &d()->m_geometry2D;
}

LightOutputNode::DisplayMode LightOutputNode::displayMode() {
    return d()->m_displayMode;
}

// WeakLightOutputNode methods

WeakLightOutputNode::WeakLightOutputNode()
{
}

WeakLightOutputNode::WeakLightOutputNode(const LightOutputNode &other)
    : d_ptr(other.d())
{
}

QSharedPointer<LightOutputNodePrivate> WeakLightOutputNode::toStrongRef() {
    return d_ptr.toStrongRef();
}

// LightOutputNodeOpenGLWorker methods

LightOutputNodeOpenGLWorker::LightOutputNodeOpenGLWorker(LightOutputNode p)
    : OpenGLWorker(p.d()->m_workerContext)
    , m_p(p)
    , m_lookupTexture2D(QOpenGLTexture::Target2D)
    , m_packet(4, 0) {
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    connect(this, &LightOutputNodeOpenGLWorker::packetReceived, this, &LightOutputNodeOpenGLWorker::onPacketReceived);

    connect(this, &LightOutputNodeOpenGLWorker::message, p.d().data(), &LightOutputNodePrivate::message);
    connect(this, &LightOutputNodeOpenGLWorker::warning, p.d().data(), &LightOutputNodePrivate::warning);
    connect(this, &LightOutputNodeOpenGLWorker::error,   p.d().data(), &LightOutputNodePrivate::error);
}

QSharedPointer<QOpenGLShaderProgram> LightOutputNodeOpenGLWorker::loadSamplerShader() {
    Q_ASSERT(QThread::currentThread() == thread());
    auto vertexString = QString{
        "#version 150\n"
        "out vec2 uv;\n"
        "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));\n"
        "void main() {\n"
        "    vec2 vertex = varray[gl_VertexID];\n"
        "    gl_Position = vec4(vertex,0.,1.);\n"
        "    uv = 0.5*(vertex+1.);\n"
        "}\n"};
    auto fragmentString = QString{
        "#version 150\n"
        "uniform sampler2D iFrame;\n"
        "uniform sampler2D iMap;\n"
        "in vec2 uv;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = texture(iFrame, texture(iMap, uv).xy);\n"
        "}\n"};

    auto shader = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

    if (!shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
        emit error("Could not compile vertex shader");
        return nullptr;
    }
    if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
        emit error("Could not compile fragment shader");
        return nullptr;
    }
    if (!shader->link()) {
        emit error("Could not link shader program");
        return nullptr;
    }

    return shader;
}

void LightOutputNodeOpenGLWorker::throwError(QString msg) {
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // LightOutputNode was deleted
    LightOutputNode p(d);

    m_connectionState = LightOutputNodeOpenGLWorker::Broken;
    emit error(msg);
    p.setNodeState(VideoNode::Broken);
    m_timer->stop();
    m_socket->close();
}

void LightOutputNodeOpenGLWorker::onStateChanged(QAbstractSocket::SocketState socketState) {
    if (socketState == QAbstractSocket::UnconnectedState) {
        if (m_connectionState == LightOutputNodeOpenGLWorker::Connected) {
            throwError("Device closed connection");
        } else if (m_connectionState == LightOutputNodeOpenGLWorker::Disconnected) {
            throwError("Could not connect");
        }
    } else if (socketState == QAbstractSocket::ConnectedState) {
        auto d = m_p.toStrongRef();
        if (d.isNull()) return; // LightOutputNode was deleted
        LightOutputNode p(d);

        p.setNodeState(VideoNode::Ready);
        m_connectionState = LightOutputNodeOpenGLWorker::Connected;
    }
    //qDebug() << "Socket is now in state" << socketState;
}

void LightOutputNodeOpenGLWorker::onReadyRead() {
    for (;;) {
        qint64 bytesAvailable = m_socket->bytesAvailable();
        if (!bytesAvailable) break;
        if (m_packetIndex < 4) {
            auto bytesToRead = qMin(bytesAvailable, (qint64)(4 - m_packetIndex));
            auto bytesRead = m_socket->read(m_packet.data() + m_packetIndex, bytesToRead);
            if (bytesRead < bytesToRead) {
                throwError("Read error");
                return;
            }
            m_packetIndex += bytesRead;
            bytesAvailable -= bytesRead;
            if (m_packetIndex == 4) {
                quint32 packetLength;
                {
                    QDataStream ds(m_packet);
                    ds.setByteOrder(QDataStream::LittleEndian);
                    ds >> packetLength;
                }
                m_packet.resize(packetLength + 4);
            }
        }
        if (m_packetIndex >= 4) {
            auto bytesToRead = qMin(bytesAvailable, (qint64)(m_packet.size() - m_packetIndex));
            auto bytesRead = m_socket->read(m_packet.data() + m_packetIndex, bytesToRead);
            if (bytesRead < bytesToRead) {
                throwError("Read error");
                return;
            }
            m_packetIndex += bytesRead;
            bytesAvailable -= bytesRead;
            if ((qint64)m_packetIndex == m_packet.size()) {
                emit packetReceived(m_packet);
                m_packet.resize(4);
                m_packetIndex = 0;
            }
        }
    }
}

void LightOutputNodeOpenGLWorker::onPacketReceived(QByteArray packet) {
    if (packet.size() == 4) return;
    auto cmd = (unsigned char)packet.at(4);
    if (cmd == 0) { // Description
        auto data = QJsonDocument::fromJson(packet.right(packet.size() - 5));
        if (data.isNull()) {
            qWarning() << "Could not parse JSON";
            return;
        }
        if (!data.isObject()) {
            qWarning() << "JSON root not an object";
            return;
        }
        auto obj = data.object();
        auto name = obj.value("name").toString();
        if (!name.isEmpty()) {
            auto d = m_p.toStrongRef();
            if (d.isNull()) return; // LightOutputNode was deleted
            LightOutputNode p(d);
            p.setName(name);
        }
        auto sizeInt = (int)obj.value("size").toDouble();
        if (sizeInt > 0) {
            emit sizeChanged(QSize(sizeInt, sizeInt));
        }
        auto sizeArray = obj.value("size").toArray();
        if (!sizeArray.isEmpty() && sizeArray.count() == 2) {
            auto width = (int)sizeArray.at(0).toDouble();
            auto height = (int)sizeArray.at(1).toDouble();
            if (width > 0 && height > 0) {
                emit sizeChanged(QSize(width, height));
            }
        }
    } else if (cmd == 1) {
        if (packet.size() != 9) {
            qWarning() << "Unexpected number of bytes";
            return;
        }
        quint32 msec;
        QDataStream ds(m_packet);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.skipRawData(5);
        ds >> msec;
        if (msec == 0) {
            m_timer->stop();
        } else {
            m_timer->setInterval(msec);
            m_timer->start();
        }
        render();
    } else if (cmd == 3) {
        if ((packet.size() - 5) % 8 != 0) {
            qWarning() << "Unexpected number of bytes";
            return;
        }

        // We make a square texture of the minimum necessary size
        // to store the output
        m_pixelCount = (packet.size() - 5) / 8;
        auto dim = (int)ceil(sqrt(m_pixelCount));

        // Load up the lookup texture
        makeCurrent();
        if (dim != m_lookupTexture2D.width()) {
            if (m_lookupTexture2D.isCreated()) {
                m_lookupTexture2D.destroy();
            }
            m_lookupTexture2D.setSize(dim, dim);
            m_lookupTexture2D.setFormat(QOpenGLTexture::RG32F);
            m_lookupTexture2D.allocateStorage(QOpenGLTexture::RG, QOpenGLTexture::Float32);
            m_lookupTexture2D.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
            m_lookupTexture2D.setWrapMode(QOpenGLTexture::ClampToEdge);
        }
        auto extraBytes = 8 * (dim * dim - m_pixelCount);
        m_packet.append(extraBytes, 0);
        m_lookupTexture2D.setData(QOpenGLTexture::RG, QOpenGLTexture::Float32, m_packet.constData() + 5);

        // Resize the FBO
        if (m_fbo->width() != dim) {
            auto fmt = QOpenGLFramebufferObjectFormat{};
            fmt.setInternalTextureFormat(GL_RGBA);
            m_fbo = QSharedPointer<QOpenGLFramebufferObject>::create(QSize(dim, dim), fmt);
        }

        // Resize the output bytearray
        m_pixelBuffer.resize(dim * dim * 4);

        // Resize the VBOs and write the lookup coordinates
        auto d = m_p.toStrongRef();
        if (d.isNull()) return; // LightOutputNode was deleted
        LightOutputNode p(d);
        {
            QMutexLocker locker(&p.d()->m_bufferLock);
            if (m_pixelCount != p.d()->m_pixelCount) {
                p.d()->m_pixelCount = m_pixelCount;
                p.d()->m_colors.bind();
                p.d()->m_colors.allocate(m_pixelCount * 4);
                p.d()->m_lookupCoordinates.bind();
                p.d()->m_lookupCoordinates.allocate(m_pixelCount * 8);
                p.d()->m_physicalCoordinates.bind();
                p.d()->m_physicalCoordinates.allocate(m_pixelCount * 8);
            }
            p.d()->m_lookupCoordinates.bind();
            p.d()->m_lookupCoordinates.write(0, m_packet.constData() + 5, m_pixelCount * 8);
            p.d()->m_lookupCoordinates.release();
        }
    } else if (cmd == 4) {
        if ((double)(packet.size() - 5) / 8 != m_pixelCount) {
            qWarning() << "Unexpected number of bytes";
            return;
        }
        // Write the physical coordinates
        auto d = m_p.toStrongRef();
        if (d.isNull()) return; // LightOutputNode was deleted
        LightOutputNode p(d);
        {
            QMutexLocker locker(&p.d()->m_bufferLock);
            p.d()->m_physicalCoordinates.bind();
            p.d()->m_physicalCoordinates.write(0, m_packet.constData() + 5, m_pixelCount * 8);
            p.d()->m_physicalCoordinates.release();
            p.d()->m_displayMode = LightOutputNode::DisplayPhysical2D;
        }
    } else if (cmd == 5) {
        auto d = m_p.toStrongRef();
        if (d.isNull()) return; // LightOutputNode was deleted
        LightOutputNode p(d);

        QImage image;
        auto result = image.loadFromData((const uchar *)m_packet.constData() + 5, m_packet.size() - 5);
        if (result) {
            QMutexLocker locker(&p.d()->m_bufferLock);
            p.d()->m_geometry2D.destroy();
            p.d()->m_geometry2D.setData(image.mirrored());
        } else {
            qWarning() << "Could not parse image data";
        }
    }
}

void LightOutputNodeOpenGLWorker::sendFrame() {
    QByteArray packetHeader(5, 0);
    QDataStream ds(&packetHeader, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint32 packetLength = 4 * m_pixelCount + 1;
    unsigned char cmdId = 2;
    ds << packetLength << cmdId;
    auto result = m_socket->write(packetHeader);
    if (result != packetHeader.size()) {
        throwError("Could not write data");
        return;
    }
    result = m_socket->write(m_pixelBuffer.data(), 4 * m_pixelCount);
    if (result != 4 * m_pixelCount) {
        throwError("Could not write data");
        return;
    }
}

void LightOutputNodeOpenGLWorker::connectToDevice(QString url) {
    m_socket->close();
    auto parts = url.split(":");
    auto port = 9001;
    if (parts.count() == 2) {
        port = parts.at(1).toInt();
    }
    m_socket->connectToHost(parts.at(0), port);
}

void LightOutputNodeOpenGLWorker::initialize() {
    Q_ASSERT(QThread::currentThread() == thread());

    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // LightOutputNode was deleted
    LightOutputNode p(d);
    p.setName("");
    auto url = p.url();

    if (m_socket == NULL) {
        m_socket = new QTcpSocket(this);
        connect(m_socket, &QAbstractSocket::stateChanged, this, &LightOutputNodeOpenGLWorker::onStateChanged);
        connect(m_socket, &QAbstractSocket::readyRead, this, &LightOutputNodeOpenGLWorker::onReadyRead);
    }
    if (m_timer == NULL) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &LightOutputNodeOpenGLWorker::render);
    }

    if (m_shader.isNull()) {
        m_shader = loadSamplerShader();
        if (m_shader.isNull()) return;
    }

    if (m_fbo.isNull()) {
        makeCurrent();

        auto fmt = QOpenGLFramebufferObjectFormat{};
        fmt.setInternalTextureFormat(GL_RGBA);
        m_fbo = QSharedPointer<QOpenGLFramebufferObject>::create(QSize(1, 1), fmt);
    }

    {
        QMutexLocker locker(&p.d()->m_bufferLock);
        if (!p.d()->m_colors.isCreated()) {
            p.d()->m_colors.create();
            p.d()->m_lookupCoordinates.create();
            p.d()->m_physicalCoordinates.create();
        }

        p.d()->m_geometry2D.destroy();
        p.d()->m_geometry2D.setSize(1, 1);
        p.d()->m_geometry2D.setFormat(QOpenGLTexture::RGBA8_UNorm);
        p.d()->m_geometry2D.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        p.d()->m_geometry2D.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
        p.d()->m_geometry2D.setWrapMode(QOpenGLTexture::Repeat);

        auto data = std::array<uint8_t,4>();
        p.d()->m_geometry2D.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
        p.d()->m_displayMode = LightOutputNode::DisplayLookup2D;
    }

    connectToDevice(url);
}

void LightOutputNodeOpenGLWorker::render() {
    Q_ASSERT(QThread::currentThread() == thread());
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // LightOutputNode was deleted
    LightOutputNode p(d);

    makeCurrent();
    GLuint texture = p.render();

    if (texture == 0) {
        qWarning() << "No frame available";
        return;
    }

    auto vao = p.chain().vao();

    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_fbo->bind();
    // VAO??
    auto size = m_fbo->size();
    glViewport(0, 0, size.width(), size.height());

    m_shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_lookupTexture2D.textureId());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

    m_shader->setUniformValue("iFrame", 0);
    m_shader->setUniformValue("iMap", 1);

    vao->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vao->release();

    glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, m_pixelBuffer.data());

    m_fbo->release();
    m_shader->release();
    glActiveTexture(GL_TEXTURE0);

    // Write the new colors to the VBO for visualization
    {
        QMutexLocker locker(&p.d()->m_bufferLock);
        p.d()->m_colors.bind();
        p.d()->m_colors.write(0, m_pixelBuffer.constData(), m_pixelCount * 4);
        p.d()->m_colors.release();
    }

    sendFrame();
}

LightOutputNodePrivate::LightOutputNodePrivate(Context *context)
    : OutputNodePrivate(context, QSize(300, 300))
    , m_geometry2D(QOpenGLTexture::Target2D)
{
}
