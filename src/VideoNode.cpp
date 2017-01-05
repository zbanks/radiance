#include "VideoNode.h"
#include "RenderContext.h"
#include <QDebug>

VideoNode::VideoNode(RenderContext *context)
    : m_context(context),
    m_displayPreviewFbo(0),
    m_renderPreviewFbo(0),
    m_prevContext(0),
    m_previewUpdated(false) {
    moveToThread(context->thread());
}

void VideoNode::render() {
    m_context->makeCurrent();
    if(m_prevContext != m_context->context) {
        initializeOpenGLFunctions(); // Placement of this function is black magic to me
        initialize();
        m_previewUpdated = false;
        m_prevContext = m_context->context;
    }

    paint();

    emit textureReady();
}

// This function is called from paint()
// to draw to the preview back-buffer.
void VideoNode::blitToPreviewFbo(QOpenGLFramebufferObject *fbo) {
    m_context->flush(); // Flush before taking the lock to speed things up a bit
    m_previewLock.lock();
    resizeFbo(&m_renderPreviewFbo, fbo->size());
    QOpenGLFramebufferObject::blitFramebuffer(m_renderPreviewFbo, fbo);
    m_context->flush();
    m_previewUpdated = true;
    m_previewLock.unlock();
}

// This function is called from the rendering thread
// to get the latest preview frame from m_displayPreviewFbo.
// It returns true if there is a new frame
bool VideoNode::swapPreview() {
    m_previewLock.lock();
    bool previewUpdated = m_previewUpdated;
    if(previewUpdated) {
        resizeFbo(&m_displayPreviewFbo, m_renderPreviewFbo->size());
        QOpenGLFramebufferObject::blitFramebuffer(m_displayPreviewFbo, m_renderPreviewFbo);
        m_previewUpdated = false;
    }
    m_previewLock.unlock();
    return previewUpdated;
}

VideoNode::~VideoNode() {
    m_context->makeCurrent();
    delete m_renderPreviewFbo;
    m_renderPreviewFbo = 0;
    delete m_displayPreviewFbo;
    m_displayPreviewFbo = 0;
    // Stop event processing, move the thread to GUI and make sure it is deleted.
    //moveToThread(QGuiApplication::instance()->thread());
}

void VideoNode::resizeFbo(QOpenGLFramebufferObject **fbo, QSize size) {
    if((*fbo)->size() != size) {
        delete *fbo;
        *fbo = new QOpenGLFramebufferObject(size);
    }
}

bool VideoNode::isMaster() {
    m_masterLock.lock();
    return m_context->master() == this;
    m_masterLock.unlock();
}

void VideoNode::setMaster(bool set) {
    m_masterLock.lock();
    VideoNode *master = m_context->master();
    if(!set && master == this) {
        m_context->setMaster(NULL);
    } else if(set && master != this) {
        m_context->setMaster(this);
    }
    m_masterLock.unlock();
}
