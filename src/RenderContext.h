#pragma once

#include <QOpenGLTexture>

class VideoNode;

// The Everpresent God Object (EGO)

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
   ~RenderContext() override;
    QOpenGLTexture *texture(VideoNode *videoNode, int chain);
};
