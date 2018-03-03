#pragma once

#include "VideoNode.h"
#include "QQuickPreviewAdapter.h"
#include <QQuickItem>
#include <QOpenGLTexture>
#include <QSGTexture>

class QQuickVideoNodePreview : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QQuickPreviewAdapter *previewAdapter READ previewAdapter WRITE setPreviewAdapter NOTIFY previewAdapterChanged)
    Q_PROPERTY(VideoNode *videoNode READ videoNode WRITE setVideoNode NOTIFY videoNodeChanged)

public:
    QQuickVideoNodePreview();
    virtual ~QQuickVideoNodePreview();

    // Watch out--
    // the pointer you pass in
    // will be cloned and stored internally
    // so don't rely in pointer values for equality checking
    // or debugging.
    // instead, check *videoNode == *otherVideoNode.
    VideoNode *videoNode();
    void setVideoNode(VideoNode *videoNode);
    // In fact, these really should take in / return VideoNode instead of VideoNode*
    // but then they would be non-nullable

    QQuickPreviewAdapter *previewAdapter();
    void setPreviewAdapter(QQuickPreviewAdapter *previewAdapter);

signals:
    void videoNodeChanged(VideoNode *videoNode);
    void previewAdapterChanged(QQuickPreviewAdapter *previewAdapter);

protected slots:
    void onWindowChanged(QQuickWindow *window);

protected:
    VideoNode *m_videoNode{};
    QQuickPreviewAdapter *m_previewAdapter{};
    QQuickWindow *m_window{};
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
};
