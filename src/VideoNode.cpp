#include "VideoNode.h"
#include "RenderContext.h"
#include <QDebug>

VideoNode::VideoNode(RenderContext *context)
    : m_context(context),
    m_previewFbo(0),
    m_displayPreviewFbo(0),
    m_renderPreviewFbo(0),
    m_prevContext(0),
    m_previewUpdated(false) {
    moveToThread(context->thread());
    m_context->addVideoNode(this);
}

void VideoNode::render() {
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
void VideoNode::blitToRenderFbo() {
    m_context->flush(); // Flush before taking the lock to speed things up a bit
    m_previewLock.lock();
    resizeFbo(&m_renderPreviewFbo, m_previewFbo->size());

    float values[] = {
        -1, -1,
        1, -1,
        -1, 1,
        1, 1
    };

    m_renderPreviewFbo->bind();
    m_context->m_premultiply->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_previewFbo->texture());
    m_context->m_premultiply->setAttributeArray(0, GL_FLOAT, values, 2);
    m_context->m_premultiply->setUniformValue("iFrame", 0);
    m_context->m_premultiply->enableAttributeArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_context->m_premultiply->disableAttributeArray(0);
    m_context->m_premultiply->release();
    QOpenGLFramebufferObject::bindDefault();

    //QOpenGLFramebufferObject::blitFramebuffer(m_renderPreviewFbo, m_previewFbo);

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

// Before deleting any FBOs and whatnot, we need to
// 1. Make sure we aren't currently rendering
// 2. Remove ourselves from the context graph
void VideoNode::beforeDestruction() {
    m_context->removeVideoNode(this);
}

VideoNode::~VideoNode() {
    delete m_previewFbo;
    m_renderPreviewFbo = 0;
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

QSet<VideoNode*> VideoNode::dependencies() {
    return QSet<VideoNode*>();
}
