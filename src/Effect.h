#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QMutex>

class Effect : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    Effect();

public slots:
    void render();
    void shutDown();

    qreal intensity();
    QString source();
    Effect *previous();

    void setIntensity(qreal value);
    void setSource(QString value);
    void setPrevious(Effect *value);

signals:
    void textureReady(int id, const QSize &size);
    void intensityChanged(qreal value);
    void sourceChanged(QString value);
    void previousChanged(Effect *value);

protected:
    QOpenGLFramebufferObject *previewFbo;

private:
    QVector<QOpenGLFramebufferObject *> m_previewFbos;
    QVector<QOpenGLShaderProgram *> m_programs;
    QOpenGLFramebufferObject *m_displayPreviewFbo;
    QOpenGLFramebufferObject *m_renderPreviewFbo;
    QOpenGLFramebufferObject *m_blankPreviewFbo;
    int m_fboIndex;

    void loadProgram(QString filename);
    void resizeFbo(QOpenGLFramebufferObject **fbo, QSize size);

    qreal m_intensity;
    QString m_source;
    QString m_prevSource;
    Effect *m_previous;

    QMutex m_intensityLock;
    QMutex m_sourceLock;
    QMutex m_previousLock;
};
