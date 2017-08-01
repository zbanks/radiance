#include "VideoNode.h"
#include "RenderContext.h"
#include "Model.h"

VideoNode::VideoNode(Model *model)
    : m_model(model) {
}

VideoNode::~VideoNode() {
}

QOpenGLTexture *VideoNode::texture(int chain) {
    return m_model->context()->texture(this, chain);
}
