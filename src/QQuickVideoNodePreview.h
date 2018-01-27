#pragma once

#include "VideoNode.h"
#include "QQuickPreviewAdapter.h"
#include <QQuickItem>
#include <QOpenGLTexture>
#include <QSGTexture>

class QQuickVideoNodePreview : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QQuickPreviewAdapter *previewAdapter READ previewAdapter WRITE setPreviewAdapter NOTIFY previewAdapterChanged)
    Q_PROPERTY(int videoNodeId READ videoNodeId WRITE setVideoNodeId NOTIFY videoNodeIdChanged)

public:
    QQuickVideoNodePreview();
    virtual ~QQuickVideoNodePreview();

    int videoNodeId();
    void setVideoNodeId(int value);

    QQuickPreviewAdapter *previewAdapter();
    void setPreviewAdapter(QQuickPreviewAdapter *previewAdapter);

signals:
    void videoNodeIdChanged(int videoNodeId);
    void previewAdapterChanged(QQuickPreviewAdapter *previewAdapter);

protected slots:
    void onWindowChanged(QQuickWindow *window);

protected:
    int m_videoNodeId;
    QQuickPreviewAdapter *m_previewAdapter;
    QQuickWindow *m_window;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
};
