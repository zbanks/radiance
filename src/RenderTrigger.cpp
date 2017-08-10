#include "RenderTrigger.h"
#include "RenderContext.h"

RenderTrigger::RenderTrigger(RenderContext *context, Model *model, int chain, QObject *obj)
        : m_context(context)
        , m_model(model)
        , m_chain(chain)
        , m_obj(obj) {
}

RenderTrigger::~RenderTrigger() {
}

void RenderTrigger::render() {
    m_context->render(m_model, m_chain);
}

bool RenderTrigger::operator==(const RenderTrigger &other) const {
    return m_context == other.m_context
        && m_model == other.m_model
        && m_chain == other.m_chain
        && m_obj == other.m_obj;
}

RenderTrigger::RenderTrigger(const RenderTrigger &other)
    : m_context(other.m_context)
    , m_model(other.m_model)
    , m_chain(other.m_chain)
    , m_obj(other.m_obj) {
}

RenderTrigger& RenderTrigger::operator=(const RenderTrigger &other) {
    m_context = other.m_context;
    m_model = other.m_model;
    m_chain = other.m_chain;
    m_obj = other.m_obj;
}

