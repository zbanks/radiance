#pragma once

#include <QOpenGLTexture>
#include "VideoNode.h"

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
   ~RenderContext() override;
    QOpenGLTexture *texture(VideoNode *videoNode, int chain);
};
