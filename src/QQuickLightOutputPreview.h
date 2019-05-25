#include <QQuickFramebufferObject>
#include "VideoNode.h"
#include "LightOutputNode.h"

class LightOutputRenderer;

class QQuickLightOutputPreview : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(LightOutputNodeSP *videoNode READ videoNode WRITE setVideoNode NOTIFY videoNodeChanged)

public:
    QQuickLightOutputPreview();

    Renderer *createRenderer() const override;

    // Use this method from other threads
    QSharedPointer<LightOutputNode> videoNodeSafe();

public slots:
    // Watch out--
    // the pointer you pass in
    // will be cloned and stored internally
    // so don't rely in pointer values for equality checking
    // or debugging.
    // instead, check *videoNode == *otherVideoNode.
    LightOutputNodeSP *videoNode();
    void setVideoNode(LightOutputNodeSP *videoNode);
    // In fact, these really should take in / return VideoNode instead of VideoNode*
    // but then they would be non-nullable
    // Also, they are not thread-safe.

signals:
    void videoNodeChanged(LightOutputNodeSP *videoNode);

protected:
    LightOutputRenderer *m_renderer;
    LightOutputNodeSP *m_videoNode{};
    QMutex m_videoNodeLock;
};
