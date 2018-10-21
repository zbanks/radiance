#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QtGlobal>
#include <QOpenGLBuffer>
#include "OutputNode.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

class LightOutputNodeOpenGLWorker;
class LightOutputNodePrivate;

class LightOutputNode
    : public OutputNode {
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

    friend class WeakLightOutputNode;
    friend class LightOutputNodeOpenGLWorker;

public:
    LightOutputNode(Context *context, QString url = "");
    LightOutputNode(const LightOutputNode &other);
    LightOutputNode *clone() const override;

    enum DisplayMode {
        DisplayLookup2D,
        DisplayPhysical2D
    };

    QJsonObject serialize() override;

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

    QMutex *bufferLock();
    // Remember to take the bufferlock before
    // calling any of these accessors:
    quint32 pixelCount();
    QOpenGLBuffer &colorsBuffer();
    QOpenGLBuffer &lookupCoordinatesBuffer();
    QOpenGLBuffer &physicalCoordinatesBuffer();
    QOpenGLTexture *geometry2DTexture();
    DisplayMode displayMode();
    // end

public slots:
    QString url();
    void setUrl(QString value);
    QString name();
    void reload();

signals:
    void urlChanged(QString value);
    void nameChanged(QString value);

private:
    LightOutputNode(QSharedPointer<LightOutputNodePrivate> other_ptr);
    QSharedPointer<LightOutputNodePrivate> d() const;

protected:
    void attachSignals();
    void setName(QString value);
};

///////////////////////////////////////////////////////////////////////////////

class WeakLightOutputNode {
public:
    WeakLightOutputNode();
    WeakLightOutputNode(const LightOutputNode &other);
    QSharedPointer<LightOutputNodePrivate> toStrongRef();

protected:
    QWeakPointer<LightOutputNodePrivate> d_ptr;
};

///////////////////////////////////////////////////////////////////////////////

class LightOutputNodeOpenGLWorker : public OpenGLWorker {
    Q_OBJECT
public:
    LightOutputNodeOpenGLWorker(LightOutputNode p);

    enum LightOutputNodeState {
        Disconnected,
        Connected,
        Broken
    };

public slots:
    void initialize();
    void render();

signals:
    void message(QString str);
    void warning(QString str);
    void error(QString str);
    void packetReceived(QByteArray packet);
    void sizeChanged(QSize size);

protected:
    QSharedPointer<QOpenGLShaderProgram> loadSamplerShader();
    void connectToDevice(QString url);
    void throwError(QString msg);
    void sendFrame();

protected slots:
    void onStateChanged(QAbstractSocket::SocketState socketState);
    void onReadyRead();
    void onPacketReceived(QByteArray packet);

private:
    WeakLightOutputNode m_p;
    QTimer *m_timer{};
    QByteArray m_pixelBuffer;
    QSharedPointer<QOpenGLShaderProgram> m_shader;
    QSharedPointer<QOpenGLFramebufferObject> m_fbo;
    QTcpSocket *m_socket{};
    LightOutputNodeState m_connectionState{Disconnected};
    QOpenGLTexture m_lookupTexture2D;
    quint32 m_pixelCount{};

    // For the radiance output protocol
    QByteArray m_packet;
    quint64 m_packetIndex{};
};

class LightOutputNodePrivate : public OutputNodePrivate {
    Q_OBJECT
public:
    LightOutputNodePrivate(Context *context);

    OpenGLWorkerContext *m_workerContext{};
    QSharedPointer<LightOutputNodeOpenGLWorker> m_worker;
    QString m_url;
    QString m_name;

    QMutex m_bufferLock;
    // Please take the bufferlock when
    // editing or using any of these:
    quint32 m_pixelCount;
    QOpenGLBuffer m_colors;
    QOpenGLBuffer m_lookupCoordinates;
    QOpenGLBuffer m_physicalCoordinates;
    QOpenGLTexture m_geometry2D;
    LightOutputNode::DisplayMode m_displayMode{LightOutputNode::DisplayLookup2D};
    // end

signals:
    void urlChanged(QString value);
    void nameChanged(QString value);
};
