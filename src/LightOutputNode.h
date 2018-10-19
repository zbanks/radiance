#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include "OutputNode.h"
#include "OpenGLWorkerContext.h"
#include "OpenGLWorker.h"

class LightOutputNodeOpenGLWorker;
class LightOutputNodePrivate;

class LightOutputNode
    : public OutputNode {
    Q_OBJECT

    friend class WeakLightOutputNode;
    friend class LightOutputNodeOpenGLWorker;

public:
    LightOutputNode(Context *context);
    LightOutputNode(const LightOutputNode &other);
    LightOutputNode *clone() const override;

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

public slots:
    QString url();
    void setUrl(QString value);

signals:
    void urlChanged(QString value);

private:
    LightOutputNode(QSharedPointer<LightOutputNodePrivate> other_ptr);
    QSharedPointer<LightOutputNodePrivate> d() const;

protected:
    void attachSignals();
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

public slots:
    void initialize();
    void render();

signals:
    void message(QString str);
    void warning(QString str);
    void error(QString str);

protected:
    QSharedPointer<QOpenGLShaderProgram> loadBlitShader();

private:
    WeakLightOutputNode m_p;
    //QTimer *m_timer{};
    QByteArray m_pixelBuffer;
    QSize m_size;
    QSharedPointer<QOpenGLShaderProgram> m_shader;
    QSharedPointer<QOpenGLFramebufferObject> m_fbo;
};

class LightOutputNodePrivate : public OutputNodePrivate {
    Q_OBJECT
public:
    LightOutputNodePrivate(Context *context);

    OpenGLWorkerContext *m_workerContext{};
    QSharedPointer<LightOutputNodeOpenGLWorker> m_worker;
    QString m_url;

signals:
    void urlChanged(QString value);
};
