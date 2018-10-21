#include <QQuickFramebufferObject>
#include "VideoNode.h"

class QQuickLightOutputPreview : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(VideoNode *videoNode READ videoNode WRITE setVideoNode NOTIFY videoNodeChanged)

public:
    Renderer *createRenderer() const;

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

signals:
    void videoNodeChanged(VideoNode *videoNode);

protected:
    VideoNode *m_videoNode{};
};
