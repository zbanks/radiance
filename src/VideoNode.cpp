#include "VideoNode.h"
#include "RenderContext.h"

VideoNode::VideoNode(Model *model)
    : m_model(model) {
}

QOpenGLTexture *VideoNode::texture(int chain) {
    return m_model->renderContext()->texture(chain);
}
