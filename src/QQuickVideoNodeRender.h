#pragma once

#include "VideoNode.h"
#include "Context.h"
#include <QQuickItem>
#include <QOpenGLTexture>
#include <QSGTexture>

class QQuickVideoNodeRender : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(Context *context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(int videoNodeId READ videoNodeId WRITE setVideoNodeId NOTIFY videoNodeIdChanged)

public:
    QQuickVideoNodeRender();
    virtual ~QQuickVideoNodeRender();

    int videoNodeId();
    void setVideoNodeId(int value);

    Context *context();
    void setContext(Context *context);

signals:
    void videoNodeIdChanged(int videoNodeId);
    void contextChanged(Context *context);

private:
    int m_videoNodeId;
    Context *m_context;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
};
