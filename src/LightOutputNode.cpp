#include "LightOutputNode.h"
#include "Context.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>
#include <array>

LightOutputNode::LightOutputNode(Context *context, QString url)
    : OutputNode(context, QSize(300, 300))
    , m_geometry2D(QOpenGLTexture::Target2D) {
    m_workerContext = new OpenGLWorkerContext(context->threaded());
    m_worker = QSharedPointer<LightOutputNodeOpenGLWorker>(new LightOutputNodeOpenGLWorker(qSharedPointerCast<LightOutputNode>(sharedFromThis())), &QObject::deleteLater);
    connect(m_worker.data(), &QObject::destroyed, m_workerContext, &QObject::deleteLater);

    m_chain->moveToWorkerContext(m_workerContext);
    connect(m_worker.data(), &LightOutputNodeOpenGLWorker::sizeChanged, this, &OutputNode::resize);

    if (!url.isEmpty()) setUrl(url);
}

QString LightOutputNode::typeName() {
    return "LightOutputNode";
}

VideoNodeSP *LightOutputNode::deserialize(Context *context, QJsonObject obj) {
    auto e = new LightOutputNode(context);
    QString url = obj.value("url").toString();
    if (!url.isEmpty()) {
        e->setUrl(url);
    }
    return new VideoNodeSP(e);
}

QJsonObject LightOutputNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("url", url());
    return o;
}

bool LightOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNodeSP *LightOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> LightOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("LightOutput", "LightOutputInstantiator.qml");
    return m;
}

QString LightOutputNode::url() {
    QMutexLocker locker(&m_stateLock);
    return m_url;
}

void LightOutputNode::reload() {
    setNodeState(VideoNode::Loading);
    auto result = QMetaObject::invokeMethod(m_worker.data(), "initialize");
    Q_ASSERT(result);
}

void LightOutputNode::setUrl(QString value) {
    {
        QMutexLocker locker(&m_stateLock);
        if(m_url == value)
            return;
        m_url = value;
    }

    reload();

    emit urlChanged(value);
}

QString LightOutputNode::name() {
    QMutexLocker locker(&m_stateLock);
    return m_name;
}

void LightOutputNode::setName(QString value) {
    {
        QMutexLocker locker(&m_stateLock);
        if(m_name == value)
            return;
        m_name = value;
    }

    emit nameChanged(value);
}

QMutex *LightOutputNode::bufferLock() {
    return &m_bufferLock;
}

quint32 LightOutputNode::pixelCount() {
    return m_pixelCount;
}

QOpenGLBuffer &LightOutputNode::colorsBuffer() {
    return m_colors;
}

QOpenGLBuffer &LightOutputNode::lookupCoordinatesBuffer() {
    return m_lookupCoordinates;
}

QOpenGLBuffer &LightOutputNode::physicalCoordinatesBuffer() {
    return m_physicalCoordinates;
}

QOpenGLTexture *LightOutputNode::geometry2DTexture() {
    return &m_geometry2D;
}

LightOutputNode::DisplayMode LightOutputNode::displayMode() {
    return m_displayMode;
}

// LightOutputNodeOpenGLWorker methods

LightOutputNodeOpenGLWorker::LightOutputNodeOpenGLWorker(QSharedPointer<LightOutputNode> p)
    : OpenGLWorker(p->m_workerContext)
    , m_p(p)
    , m_lookupTexture2D(QOpenGLTexture::Target2D)
    , m_packet(4, 0) {
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    connect(this, &LightOutputNodeOpenGLWorker::packetReceived, this, &LightOutputNodeOpenGLWorker::onPacketReceived);

    connect(this, &LightOutputNodeOpenGLWorker::message, p.data(), &LightOutputNode::message);
    connect(this, &LightOutputNodeOpenGLWorker::warning, p.data(), &LightOutputNode::warning);
    connect(this, &LightOutputNodeOpenGLWorker::error,   p.data(), &LightOutputNode::error);
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
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // LightOutputNode was deleted

    m_connectionState = LightOutputNodeOpenGLWorker::Broken;
    emit error(msg);
    p->setNodeState(VideoNode::Broken);
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
        auto p = m_p.toStrongRef();
        if (p.isNull()) return; // LightOutputNode was deleted

        p->setNodeState(VideoNode::Ready);
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
            emit warning("Could not parse JSON in \"description\" packet");
            return;
        }
        if (!data.isObject()) {
            emit warning("JSON root not an object in \"description\" packet");
            return;
        }
        auto obj = data.object();
        auto name = obj.value("name").toString();
        if (!name.isEmpty()) {
            auto p = m_p.toStrongRef();
            if (p.isNull()) return; // LightOutputNode was deleted
            p->setName(name);
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
            emit warning("Unexpected number of bytes in \"get frame\" packet");
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
            emit warning("Unexpected number of bytes in \"lookup coordinates 2D\" packet");
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
        auto p = m_p.toStrongRef();
        if (p.isNull()) return; // LightOutputNode was deleted
        {
            QMutexLocker locker(&p->m_bufferLock);
            if (m_pixelCount != p->m_pixelCount) {
                p->m_pixelCount = m_pixelCount;
                p->m_colors.bind();
                p->m_colors.allocate(m_pixelCount * 4);
                p->m_lookupCoordinates.bind();
                p->m_lookupCoordinates.allocate(m_pixelCount * 8);
                p->m_physicalCoordinates.bind();
                p->m_physicalCoordinates.allocate(m_pixelCount * 8);
            }
            p->m_lookupCoordinates.bind();
            p->m_lookupCoordinates.write(0, m_packet.constData() + 5, m_pixelCount * 8);
            p->m_lookupCoordinates.release();
        }
    } else if (cmd == 4) {
        if ((double)(packet.size() - 5) / 8 != m_pixelCount) {
            emit warning("Unexpected number of bytes in \"physical coordinates 2D\" packet");
            return;
        }
        // Write the physical coordinates
        auto p = m_p.toStrongRef();
        if (p.isNull()) return; // LightOutputNode was deleted
        {
            QMutexLocker locker(&p->m_bufferLock);
            p->m_physicalCoordinates.bind();
            p->m_physicalCoordinates.write(0, m_packet.constData() + 5, m_pixelCount * 8);
            p->m_physicalCoordinates.release();
            p->m_displayMode = LightOutputNode::DisplayPhysical2D;
        }
    } else if (cmd == 5) {
        auto p = m_p.toStrongRef();
        if (p.isNull()) return; // LightOutputNode was deleted

        QImage image;
        auto result = image.loadFromData((const uchar *)m_packet.constData() + 5, m_packet.size() - 5);
        if (result) {
            QMutexLocker locker(&p->m_bufferLock);
            p->m_geometry2D.destroy();
            p->m_geometry2D.setData(image.mirrored());
        } else {
            emit warning("Could not parse image data in \"geometry 2D\" packet");
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
    auto port = 11647;
    if (parts.count() == 2) {
        port = parts.at(1).toInt();
    }
    m_socket->connectToHost(parts.at(0), port);
}

void LightOutputNodeOpenGLWorker::initialize() {
    Q_ASSERT(QThread::currentThread() == thread());

    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // LightOutputNode was deleted
    p->setName("");
    auto url = p->url();

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
        QMutexLocker locker(&p->m_bufferLock);
        m_pixelCount = 0;
        p->m_pixelCount = 0;
        if (!p->m_colors.isCreated()) {
            p->m_colors.create();
            p->m_lookupCoordinates.create();
            p->m_physicalCoordinates.create();
        }

        p->m_geometry2D.destroy();
        p->m_geometry2D.setSize(1, 1);
        p->m_geometry2D.setFormat(QOpenGLTexture::RGBA8_UNorm);
        p->m_geometry2D.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        p->m_geometry2D.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
        p->m_geometry2D.setWrapMode(QOpenGLTexture::Repeat);

        auto data = std::array<uint8_t,4>();
        p->m_geometry2D.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
        p->m_displayMode = LightOutputNode::DisplayLookup2D;
    }

    connectToDevice(url);
}

void LightOutputNodeOpenGLWorker::render() {
    Q_ASSERT(QThread::currentThread() == thread());
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // LightOutputNode was deleted

    makeCurrent();
    GLuint texture = p->render();

    if (texture == 0) {
        qWarning() << "No frame available";
        return;
    }

    auto vao = p->chain()->vao();

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
        QMutexLocker locker(&p->m_bufferLock);
        p->m_colors.bind();
        p->m_colors.write(0, m_pixelBuffer.constData(), m_pixelCount * 4);
        p->m_colors.release();
    }

    sendFrame();
}
