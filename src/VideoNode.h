#pragma once

#include <QObject>
#include <QOpenGLTexture>

class Model;

class VideoNode : public QObject {
    Q_OBJECT

public:
    VideoNode(Model *model);
   ~VideoNode() override;

    QOpenGLTexture *texture(int chain);

protected:
    Model *m_model;
};
