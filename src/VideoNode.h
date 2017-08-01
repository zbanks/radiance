#pragma once

#include <QObject>
#include <QOpenGLTexture>

#include "Model.h"

class VideoNode : public QObject {
    Q_OBJECT

public:
    VideoNode(Model *model);
   ~VideoNode() override;

    QOpenGLTexture *texture(int chain);

protected:
    Model *m_model;
};
