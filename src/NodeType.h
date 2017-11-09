#pragma once

#include "Chain.h"
#include <QObject>
#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QOpenGLFunctions>
#include <QMutex>
#include <atomic>
#include <mutex>
#include <thread>

//#include "NodeRegistry.h"
class NodeRegistry;
class VideoNode;

class NodeType : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged);
    Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged);
    Q_PROPERTY(int inputCount READ inputCount WRITE setInputCount NOTIFY inputCountChanged);
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged);
    Q_PROPERTY(NodeRegistry * registry READ registry);
public:
    NodeType(NodeRegistry *r = nullptr, QObject *p = nullptr);
   ~NodeType() override;

public slots:
    QString name() const;
    QString description() const;
    QString author() const;
    int     inputCount() const;
    bool    ready() const;
    NodeRegistry *registry() const;

    void setName(QString);
    void setDescription(QString);
    void setAuthor(QString);
    void setInputCount(int);
    void isReady();
    void setReady(bool);

    virtual VideoNode* create(QString arg);
signals:
    void nameChanged(QString);
    void descriptionChanged(QString);
    void authorChanged(QString);
    void inputCountChanged(int);
    void readyChanged(bool);
protected:
    QString m_name{};
    QString m_description{};
    QString m_author{};
    int     m_inputCount{0};
    std::atomic<bool> m_ready{false};
    NodeRegistry *m_registry{};
};
