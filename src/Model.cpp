#include "Model.h"
#include "EffectNode.h"
#include "main.h"

Model::Model() {
}

Model::~Model() {
}

void Model::addVideoNode(VideoNode *videoNode) {
    if (!m_vertices.contains(videoNode)) {
        m_vertices.append(videoNode);
        emit videoNodeAdded(videoNode);
    }
}

void Model::removeVideoNode(VideoNode *videoNode) {
    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        if (edge->fromVertex == videoNode || edge->toVertex == videoNode) {
            auto edgeCopy = *edge;
            m_edges.erase(edge);
            emit edgeRemoved(edgeCopy);
        }
    }

    int count = m_vertices.removeAll(videoNode);
    if (count > 0) {
        emit videoNodeRemoved(videoNode);
    }
}

void Model::addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    Q_ASSERT(fromVertex != nullptr);
    Q_ASSERT(toVertex != nullptr);
    Q_ASSERT(toInput >= 0);
    Q_ASSERT(m_vertices.contains(fromVertex));
    Q_ASSERT(m_vertices.contains(toVertex));

    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        if (edge->toVertex == toVertex && edge->toInput == toInput) {
            if (edge->fromVertex == fromVertex)
                return;
            Edge edgeCopy = *edge;
            m_edges.erase(edge);
            emit edgeRemoved(edgeCopy);
        }
    }

    Edge newEdge = {
        .fromVertex = fromVertex,
        .toVertex = toVertex,
        .toInput = toInput,
    };
    m_edges.append(newEdge);
    emit edgeAdded(newEdge);
}

void Model::removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    Q_ASSERT(fromVertex != nullptr);
    Q_ASSERT(toVertex != nullptr);
    Q_ASSERT(toInput >= 0);
    Q_ASSERT(m_vertices.contains(fromVertex));
    Q_ASSERT(m_vertices.contains(toVertex));

    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        if (edge->fromVertex == fromVertex &&
            edge->toVertex == toVertex &&
            edge->toInput == toInput) {

            auto edgeCopy = *edge;
            m_edges.erase(edge);
            emit edgeRemoved(edgeCopy);
        }
    }
}
