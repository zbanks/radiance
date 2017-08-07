#include "VideoNode.h"
#include "RenderContext.h"
#include "Model.h"

VideoNode::VideoNode(RenderContext *context, int inputCount)
    : m_context(context)
    , m_inputCount(inputCount) {
}

VideoNode::VideoNode(const VideoNode &other)
    : m_context(other.m_context)
    , m_inputCount(other.m_inputCount) {
}

VideoNode::~VideoNode() {
}

int VideoNode::inputCount() {
    return m_inputCount;
}

QSize VideoNode::size(int chain) {
    return m_context->chainSize(chain);
}

RenderContext *VideoNode::context() {
    return m_context;
}
