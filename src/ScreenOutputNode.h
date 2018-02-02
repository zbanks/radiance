#pragma once

#include "OutputNode.h"
#include <QWindow>

class ScreenOutputNode
    : public OutputNode {
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged);

public:
    ScreenOutputNode(Context *context, QSize chainSize);
    ScreenOutputNode(const ScreenOutputNode &other);
    ~ScreenOutputNode();

public slots:
    bool visible();
    void setVisible(bool visible);

signals:
    void visibleChanged(bool visible);

protected:
    QWindow *m_outputWindow;
};

//////////////////////////////////////////////////////////////////////////////

class ScreenOutputNodeFactory : public VideoNodeFactory {

public:
    QString typeName() override;
    VideoNode *deserialize(Context *context, QJsonObject obj) override;
};
