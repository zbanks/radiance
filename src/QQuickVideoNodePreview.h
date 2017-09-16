#pragma once

#include "VideoNode.h"
#include "Context.h"
#include <QQuickItem>
#include <QOpenGLTexture>
#include <QSGTexture>

class QQuickVideoNodePreview : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(Context *context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(int videoNodeId READ videoNodeId WRITE setVideoNodeId NOTIFY videoNodeIdChanged)

public:
    QQuickVideoNodePreview();
    virtual ~QQuickVideoNodePreview();

    int videoNodeId();
    void setVideoNodeId(int value);

    Context *context();
    void setContext(Context *context);

signals:
    void videoNodeIdChanged(int videoNodeId);
    void contextChanged(Context *context);

protected slots:
    void onWindowChanged(QQuickWindow *window);

protected:
    int m_videoNodeId;
    Context *m_context;
    QQuickWindow *m_window;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
};
