#include "QQuickPreviewAdapter.h"
#include "Model.h"
#include <memory>
#include <QThread>

QQuickPreviewAdapter::QQuickPreviewAdapter(QSize size)
    : m_previewSize(size)
    , m_previewChain(new Chain(size), &QObject::deleteLater)
{
}

QQuickPreviewAdapter::~QQuickPreviewAdapter() {
    if (m_model != nullptr) {
        (*m_model)->removeChain(m_previewChain);
    }
}

ModelSP *QQuickPreviewAdapter::model() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_model;
}

void QQuickPreviewAdapter::setModel(ModelSP *model) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_model != model) {
        if (m_model != nullptr) {
            (*m_model)->removeChain(m_previewChain);
        }
        m_model = model;
        if (m_model != nullptr) {
            (*m_model)->addChain(m_previewChain);
        }
        emit modelChanged(model);
    }
}

QSize QQuickPreviewAdapter::previewSize() {
    QMutexLocker locker(&m_previewLock);
    return m_previewSize;
}

void QQuickPreviewAdapter::setPreviewSize(QSize size) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (size != m_previewSize) {
        {
            QMutexLocker locker(&m_previewLock);
            m_previewSize = size;
            ChainSP previewChain(new Chain(size));
            if (m_model != nullptr) {
                (*m_model)->removeChain(m_previewChain);
                (*m_model)->addChain(previewChain);
            }
            m_previewChain = previewChain;
        }
        emit previewSizeChanged(size);
    }
}

QQuickWindow *QQuickPreviewAdapter::previewWindow() {
    return m_previewWindow;
}

void QQuickPreviewAdapter::setPreviewWindow(QQuickWindow *window) {
    Q_ASSERT(QThread::currentThread() == thread());
    {
        QMutexLocker locker(&m_previewLock);
        if (m_previewWindow ) {
            disconnect(m_previewWindow, &QQuickWindow::beforeSynchronizing, this, &QQuickPreviewAdapter::onBeforeSynchronizing);
        }
        m_previewWindow = window;
        if (m_previewWindow ) {
            connect(m_previewWindow, &QQuickWindow::beforeSynchronizing, this, &QQuickPreviewAdapter::onBeforeSynchronizing, Qt::DirectConnection);
        }
    }
    emit previewWindowChanged(window);
}

void QQuickPreviewAdapter::onBeforeSynchronizing() {
    if (m_model != nullptr) {
        auto modelCopy = (*m_model)->createCopyForRendering();
        m_lastPreviewRender = modelCopy.render(m_previewChain);
    }
}

GLuint QQuickPreviewAdapter::previewTexture(VideoNodeSP *videoNode) {
    return m_lastPreviewRender.value(*videoNode, 0);
}
