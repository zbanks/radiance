#pragma once

#include "Model.h"
#include "Chain.h"
#include "VideoNode.h"
#include <QList>
#include <QQuickWindow>

class QQuickPreviewAdapter : public QObject {
    Q_OBJECT
    Q_PROPERTY(ModelSP *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize NOTIFY previewSizeChanged)
    Q_PROPERTY(QQuickWindow *previewWindow READ previewWindow WRITE setPreviewWindow NOTIFY previewWindowChanged)

public:
    QQuickPreviewAdapter(QSize size=QSize(300, 300));
   ~QQuickPreviewAdapter() override;

public slots:
    ModelSP *model();
    void setModel(ModelSP *model);
    QSize previewSize(); // thread-safe
    void setPreviewSize(QSize size);
    QQuickWindow *previewWindow(); // thread-safe
    void setPreviewWindow(QQuickWindow *window);

    // Use this method to retrieve
    // rendered preview textures
    GLuint previewTexture(VideoNodeSP *videoNode);

protected slots:
    void onBeforeSynchronizing();

signals:
    void previewSizeChanged(QSize size);
    void modelChanged(ModelSP *model);
    void previewWindowChanged(QQuickWindow *window);

protected:
    // m_model needs to be a ModelSP since there are QML properties that fetch it
    // This one ModelSP pointer can be shared by all of the UI and not cause problems
    ModelSP *m_model{};
    QSize m_previewSize;
    QSharedPointer<Chain> m_previewChain;
    QQuickWindow *m_previewWindow{};
    QMap<QSharedPointer<VideoNode>, GLuint> m_lastPreviewRender;
    QMutex m_previewLock;
};
