#pragma once

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QMutex>

class RenderContext;

class Effect : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    Effect(RenderContext *context);
    ~Effect();
    QOpenGLFramebufferObject *m_displayPreviewFbo;
    bool loadProgram(QString name);
 
public slots:
    void render();

    qreal intensity();
    Effect *previous();
    bool isMaster();

    void setIntensity(qreal value);
    void setPrevious(Effect *value);
    void setMaster(bool set);

    bool swapPreview();

signals:
    void textureReady();
    void intensityChanged(qreal value);
    void sourceChanged(QString value);
    void previousChanged(Effect *value);

protected:
    QOpenGLFramebufferObject *previewFbo;

private:
    RenderContext *m_context;
    QOpenGLContext *m_prevContext;

    QVector<QOpenGLFramebufferObject *> m_previewFbos;
    QVector<QOpenGLShaderProgram *> m_programs;
    QOpenGLFramebufferObject *m_renderPreviewFbo;
    QOpenGLFramebufferObject *m_blankPreviewFbo;
    int m_fboIndex;

    void initialize();
    void resizeFbo(QOpenGLFramebufferObject **fbo, QSize size);

    qreal m_intensity;
    QString m_source;
    QString m_prevSource;
    Effect *m_previous;

    QMutex m_intensityLock;
    QMutex m_programLock;
    QMutex m_previousLock;
    QMutex m_masterLock;
    QMutex m_previewLock;

    bool m_previewUpdated;
    bool m_regeneratePreviewFbos;
};
