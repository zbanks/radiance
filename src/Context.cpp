#include "main.h"
#include "Context.h"
#include "Model.h"
#include <memory>

Context::Context(bool hasPreview) 
    : m_hasPreview(hasPreview)
    , m_previewSize(QSize(300, 300)) {

    if (m_hasPreview) {
        m_previewChain = QSharedPointer<Chain>(new Chain(m_previewSize));
    }
}

Context::~Context() {
}

Model *Context::model() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_model;
}

void Context::setModel(Model *model) {
    Q_ASSERT(QThread::currentThread() == thread());
    m_model = model;
    chainsChanged();
    emit modelChanged(model);
}

QSize Context::previewSize() {
    Q_ASSERT(m_hasPreview);
    QMutexLocker locker(&m_previewLock);
    return m_previewSize;
}

void Context::setPreviewSize(QSize size) {
    Q_ASSERT(m_hasPreview);
    Q_ASSERT(QThread::currentThread() == thread());
    if (size != m_previewSize) {
        {
            QMutexLocker locker(&m_previewLock);
            m_previewSize = size;
            QSharedPointer<Chain> previewChain(new Chain(size));
            m_previewChain = previewChain;
        }
        chainsChanged();
        emit previewSizeChanged(size);
    }
}

QQuickWindow *Context::previewWindow() {
    return m_previewWindow;
}

void Context::setPreviewWindow(QQuickWindow *window) {
    Q_ASSERT(m_hasPreview);
    Q_ASSERT(QThread::currentThread() == thread());
    {
        QMutexLocker locker(&m_previewLock);
        disconnect(m_previewWindow, &QQuickWindow::beforeSynchronizing, this, &Context::onBeforeSynchronizing);
        m_previewWindow = window;
        connect(m_previewWindow, &QQuickWindow::beforeSynchronizing, this, &Context::onBeforeSynchronizing, Qt::DirectConnection);
    }
    emit previewWindowChanged(window);
}

void Context::onBeforeSynchronizing() {
    Q_ASSERT(m_hasPreview);
    auto modelCopy = m_model->createCopyForRendering();
    m_lastPreviewRender = modelCopy.render(m_previewChain);
    m_model->copyBackRenderStates(m_previewChain, &modelCopy);
}

GLuint Context::previewTexture(int videoNodeId) {
    Q_ASSERT(m_hasPreview);
    return m_lastPreviewRender.value(videoNodeId, 0);
}

void Context::onRenderRequested() {
    auto output = qobject_cast<Output *>(sender());
    auto name = output->name();
    auto modelCopy = m_model->createCopyForRendering();
    auto vnId = modelCopy.outputs.value(name, 0);
    GLuint textureId = 0;
    if (vnId != 0) { // Don't bother rendering this chain
        // if it is not connected
        auto chain = output->chain();
        auto result = modelCopy.render(chain);
        textureId = result.value(vnId, 0);
        m_model->copyBackRenderStates(chain, &modelCopy);
    }
    output->renderReady(textureId);
}

QList<QSharedPointer<Chain>> Context::chains() {
    auto o = outputs();
    QList<QSharedPointer<Chain>> chains;
    if (m_hasPreview) {
        chains.append(m_previewChain);
    }
    for (int i=0; i<o.count(); i++) {
        chains.append(o.at(i)->chain());
    }
    return chains;
}

void Context::chainsChanged() {
    m_model->setChains(chains());
}

QList<Output *> Context::outputs() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_outputs;
}

void Context::setOutputs(QList<Output *> outputs) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_outputs != outputs) {
        // TODO I am lazy
        for (int i=0; i<m_outputs.count(); i++) {
            disconnect(m_outputs.at(i), &Output::renderRequested, this, &Context::onRenderRequested);
            disconnect(m_outputs.at(i), &Output::chainChanged, this, &Context::chainsChanged);
        }
        m_outputs = outputs;
        for (int i=0; i<outputs.count(); i++) {
            connect(outputs.at(i), &Output::renderRequested, this, &Context::onRenderRequested, Qt::DirectConnection);
            connect(outputs.at(i), &Output::chainChanged, this, &Context::chainsChanged);
        }
        chainsChanged();
        emit outputsChanged(outputs);
    }
}
