#pragma once

#include "VideoNode.h"
#include <QQuickItem>
#include <QOpenGLTexture>
#include <QSGTexture>

class QQuickVideoNodeRender : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(int videoNodeId READ videoNodeId WRITE setVideoNodeId NOTIFY videoNodeIdChanged)
    Q_PROPERTY(int chain READ chain WRITE setChain NOTIFY chainChanged)

public:
    QQuickVideoNodeRender();
    virtual ~QQuickVideoNodeRender();

    VnId videoNodeId();
    int chain();
    void setVideoNodeId(VnId value);
    void setChain(int chain);

signals:
    void videoNodeIdChanged(VnId videoNodeId);
    void chainChanged(int chain);

private:
    int m_chain;
    VnId m_videoNodeId;

protected:
    QSharedPointer<RenderContext> m_context;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    QQuickWindow *m_window;

protected slots:
    void onWindowChanged(QQuickWindow *window);
};
