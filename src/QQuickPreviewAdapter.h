#pragma once

#include "Model.h"
#include <QList>
#include <QQuickWindow>

class QQuickPreviewAdapter : public QObject {
    Q_OBJECT
    Q_PROPERTY(Model *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize NOTIFY previewSizeChanged)
    Q_PROPERTY(QQuickWindow *previewWindow READ previewWindow WRITE setPreviewWindow NOTIFY previewWindowChanged)

public:
    QQuickPreviewAdapter(QSize size=QSize(300, 300));
   ~QQuickPreviewAdapter() override;

public slots:
    Model *model();
    void setModel(Model *model);
    QSize previewSize(); // thread-safe
    void setPreviewSize(QSize size);
    QQuickWindow *previewWindow(); // thread-safe
    void setPreviewWindow(QQuickWindow *window);

    // Use this method to retrieve
    // rendered preview textures
    GLuint previewTexture(VideoNode *videoNode);

protected slots:
    void onBeforeSynchronizing();

signals:
    void previewSizeChanged(QSize size);
    void modelChanged(Model *model);
    void previewWindowChanged(QQuickWindow *window);

protected:
    Model *m_model{};
    QSize m_previewSize;
    Chain m_previewChain;
    QQuickWindow *m_previewWindow{};
    QMap<VideoNode, GLuint> m_lastPreviewRender;
    QMutex m_previewLock;
};
