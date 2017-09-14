#include "Context.h"

Renderer::Renderer() {
}

Renderer::~Renderer() {
}

QList<Output *> Renderer::outputs() {
    return m_outputs;
}

void Renderer::addOutput(Output * output) {
    if (!m_outputs.contains(output)) {
        m_outputs.append(output);
        connect(output, &Output::renderRequested, this, &Renderer::onRenderRequested, Qt::DirectConnection);
        emit outputsChanged(m_outputs);
    }
}

void Renderer::removeOutput(Output * output) {
    if (m_outputs.contains(output)) {
        m_outputs.removeAll(output);
        disconnect(output, &Output::renderRequested, this, &Renderer::onRenderRequested);
        emit outputsChanged(m_outputs);
    }
}

void Renderer::onRenderRequested() {
    auto output = qobject_cast<Output *>(sender());
    emit renderRequested(output);
}
