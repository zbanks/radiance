#pragma once

#include <QOpenGLTexture>
#include <QSharedPointer>
#include <QVector>

class VideoNode;

// The Everpresent God Object (EGO)

class RenderContext : public QObject {
    Q_OBJECT

public:
    RenderContext();
   ~RenderContext() override;
    void initialize();
    QOpenGLTexture *texture(int chain, VideoNode *videoNode);
    int chainCount();
    QSize chainSize(int chain);
    QSharedPointer<QOpenGLTexture> noiseTexture(int chain);
    QSharedPointer<QOpenGLTexture> blankTexture();

private:
    void createNoiseTextures();
    void createBlankTexture();
    bool m_initialized;
    QVector<QSharedPointer<QOpenGLTexture> > m_noiseTextures;
    QSharedPointer<QOpenGLTexture> m_blankTexture;
};
