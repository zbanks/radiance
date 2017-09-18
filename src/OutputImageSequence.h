#pragma once

#include "Output.h"
#include <QQuickItem>

class OutputImageSequenceWorker;

class OutputImageSequence : public Output {
    friend class OutputImageSequenceWorker;

    Q_OBJECT
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(qreal fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    OutputImageSequence();
   ~OutputImageSequence() override;

    void display(GLuint textureId) override;
    QSize size();
    void setSize(QSize size);
    qreal fps();
    void setFps(qreal fps);
    QString filename();
    void setFilename(QString filename);
    bool enabled();
    void setEnabled(bool enabled);

signals:
    void sizeChanged(QSize size);
    void fpsChanged(qreal fps);
    void filenameChanged(QString filename);
    void enabledChanged(bool enabled);

protected:
    QSharedPointer<OutputImageSequenceWorker> m_worker;
    QSize m_size;
    int m_frameDelay;
    QString m_filename;
    QThread m_thread;
    bool m_enabled;
    QSharedPointer<OpenGLWorkerContext> m_context;

    void join();

protected slots:
    void onDestroyed();
};
