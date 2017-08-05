#include "VideoNode.h"
#include "RenderContext.h"
#include "Model.h"

VideoNode::VideoNode(RenderContext *context, int inputCount)
    : m_context(context)
    , m_inputCount(inputCount)
    , m_textures(context->chainCount()) {
}

VideoNode::~VideoNode() {
}

int VideoNode::inputCount() {
    return m_inputCount;
}

QSharedPointer<QOpenGLTexture> VideoNode::texture(int chain) {
    return m_textures.at(chain);
}
