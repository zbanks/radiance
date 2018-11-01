#include <QQuickFramebufferObject>
#include "VideoNode.h"

class LightOutputRenderer;
class LightOutputNode;

class QQuickLightOutputPreview : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(VideoNode *videoNode READ videoNode WRITE setVideoNode NOTIFY videoNodeChanged)

public:
    QQuickLightOutputPreview();

    Renderer *createRenderer() const override;

    // Use this method from other threads
    LightOutputNode *videoNodeSafe();

public slots:
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
    // Also, they are not thread-safe.

signals:
    void videoNodeChanged(VideoNode *videoNode);

protected:
    LightOutputRenderer *m_renderer;
    VideoNode *m_videoNode{};
    QMutex m_videoNodeLock;
};
