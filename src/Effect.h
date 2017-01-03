#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>

class EffectUI;

class Effect : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    Effect(EffectUI *e);

public slots:
    void render();
    void shutDown();

signals:
    void textureReady(int id, const QSize &size);

private:
    QVector<QOpenGLFramebufferObject *> m_previewFbos;
    QVector<QOpenGLShaderProgram *> m_programs;
    QOpenGLFramebufferObject * m_displayPreviewFbo;
    QOpenGLFramebufferObject * m_renderPreviewFbo;
    QOpenGLFramebufferObject * m_blankPreviewFbo;
    EffectUI *e;
    QString m_source;
    int m_fboIndex;

    void loadProgram(QString filename);
    void resizeFbo(QOpenGLFramebufferObject **fbo, QSize size);
};
