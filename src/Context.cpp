#include "main.h"
#include "Context.h"
#include "Model.h"
#include <memory>

Context::Context() {
    connect(&m_renderer, &Renderer::renderRequested, this, &Context::onRenderRequested);
    connect(&m_renderer, &Renderer::outputsChanged, this, &Context::onOutputsChanged);
}

Context::~Context() {
}

Model *Context::model() {
    return &m_model;
}

Renderer *Context::renderer() {
    return &m_renderer;
}

void Context::onRenderRequested(Output *output) {
    auto result = render(output->chain());
    output->display(result);
}

void Context::onOutputsChanged(QList<Output *> outputs) {
    QList<QSharedPointer<Chain>> chains;
    for (int i=0; i<outputs.count(); i++) {
        chains.append(outputs.at(i)->chain());
    }
    m_model.setChains(chains);
}

QMap<int, GLuint> Context::render(QSharedPointer<Chain> chain) {
    //qDebug() << "RENDER!" << chain;

    auto m = m_model.createCopyForRendering();

    // inputs is parallel to vertices
    // and contains the VideoNodes connected to the
    // corresponding vertex's inputs
    QVector<QVector<int>> inputs;

    // Create a list of -1's
    for (int i=0; i<m.vertices.count(); i++) {
        auto inputCount = m.vertices.at(i)->inputCount();
        inputs.append(QVector<int>(inputCount, -1));
    }

    Q_ASSERT(m.fromVertex.count() == m.toVertex.count());
    Q_ASSERT(m.fromVertex.count() == m.toInput.count());
    for (int i = 0; i < m.toVertex.count(); i++) {
        auto to = m.toVertex.at(i);
        if (to >= 0) {
            inputs[to][m.toInput.at(i)] = m.fromVertex.at(i);
        }
    }

    QVector<GLuint> resultTextures(m.vertices.count(), 0);

    for (int i=0; i<m.vertices.count(); i++) {
        auto vertex = m.vertices.at(i);
        QVector<GLuint> inputTextures(vertex->inputCount(), chain->blankTexture());
        for (int j=0; j<vertex->inputCount(); j++) {
            auto fromVertex = inputs.at(i).at(j);
            if (fromVertex != -1) {
                auto inpTexture = resultTextures.at(fromVertex);
                if (inpTexture != 0) {
                    inputTextures[j] = inpTexture;
                }
            }
        }
        resultTextures[i] = vertex->paint(chain, inputTextures);
        //qDebug() << vertex << "wrote texture" << vertex->texture(chain);
    }

    m_model.copyBackRenderStates(chain, m.origVertices, m.vertices);

    QMap<int, GLuint> result;
    for (int i=0; i<resultTextures.count(); i++) {
        if (resultTextures.at(i) != 0 && m.vertices.at(i)->id() != 0) {
            result.insert(m.vertices.at(i)->id(), resultTextures.at(i));
        }
    }
    return result;
}

