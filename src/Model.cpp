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
        regenerateGraph();
        emit videoNodeAdded(videoNode);
        emitGraphChanged();
    }
}

void Model::removeVideoNode(VideoNode *videoNode) {
    QList<Edge> removed;
    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        if (edge->fromVertex == videoNode || edge->toVertex == videoNode) {
            auto edgeCopy = *edge;
            m_edges.erase(edge);
            removed.append(edgeCopy);
        }
    }

    int count = m_vertices.removeAll(videoNode);

    if (removed.count() > 0 || count > 0) {
        regenerateGraph();
        for (auto edge = removed.begin(); edge != removed.end(); edge++) {
            emit edgeRemoved(edge->fromVertex, edge->toVertex, edge->toInput);
        }
        if (count > 0) {
            emit videoNodeRemoved(videoNode);
        }
        emitGraphChanged();
    }

}

void Model::addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    Q_ASSERT(fromVertex != nullptr);
    Q_ASSERT(toVertex != nullptr);
    Q_ASSERT(toInput >= 0);
    Q_ASSERT(m_vertices.contains(fromVertex));
    Q_ASSERT(m_vertices.contains(toVertex));

    QList<Edge> removed;

    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        if (edge->toVertex == toVertex && edge->toInput == toInput) {
            if (edge->fromVertex == fromVertex)
                return;
            auto edgeCopy = *edge;
            m_edges.erase(edge);
            removed.append(edgeCopy);
        }
    }

    Edge newEdge = {
        .fromVertex = fromVertex,
        .toVertex = toVertex,
        .toInput = toInput,
    };
    m_edges.append(newEdge);
    regenerateGraph();
    for (auto edge = removed.begin(); edge != removed.end(); edge++) {
        emit edgeRemoved(edge->fromVertex, edge->toVertex, edge->toInput);
    }
    emit edgeAdded(newEdge.fromVertex, newEdge.toVertex, newEdge.toInput);
    emitGraphChanged();
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
            regenerateGraph();
            emit edgeRemoved(edgeCopy.fromVertex, edgeCopy.toVertex, edgeCopy.toInput);
            emitGraphChanged();
            return;
        }
    }
}

void Model::regenerateGraph() {
    // TODO store this object and only return a new one
    // if the graph has changed
    // (making these is sort of expensive)
    m_graph = ModelGraph(m_vertices, m_edges);
}

void Model::emitGraphChanged() {
    emit graphChanged(m_graph);
    emit qmlGraphChanged();
}

ModelGraph Model::graph() {
    return m_graph;
}

ModelGraph *Model::qmlGraph() {
    ModelGraph *mg = new ModelGraph(m_graph);
    QQmlEngine::setObjectOwnership(mg, QQmlEngine::JavaScriptOwnership);
    return mg;
}

// ModelGraph methods

ModelGraph::ModelGraph() {
}

ModelGraph::ModelGraph(QList<VideoNode *> vertices, QList<Edge> edges)
    : m_vertices(QVector<VideoNode *>::fromList(vertices)) {
    QMap<VideoNode *, int> map;
    int i = 0;
    for (int i=0; i<m_vertices.count(); i++) {
        map.insert(m_vertices.at(i), i);
    }
    for (auto edge = edges.begin(); edge != edges.end(); edge++) {
        ModelGraphEdge mge {
            .fromVertex = map.value(edge->fromVertex, -1),
            .toVertex = map.value(edge->toVertex, -1),
            .toInput = edge->toInput,
        };
        m_edges.append(mge);
    }
}

ModelGraph::ModelGraph(const ModelGraph &other)
    : m_vertices(other.m_vertices)
    , m_edges(other.m_edges) {
}

QVector<VideoNode *> ModelGraph::vertices() {
    return m_vertices;
}

QVector<ModelGraphEdge> ModelGraph::edges() {
    return m_edges;
}

QQmlListProperty<VideoNode> ModelGraph::qmlVertices() {
    return QQmlListProperty<VideoNode>(this, this, &ModelGraph::vertexCount, &ModelGraph::vertexAt);
}

int ModelGraph::vertexCount() const { 
    return m_vertices.count();
}

VideoNode *ModelGraph::vertexAt(int index) const {
    return m_vertices.at(index);
}

QVariantList ModelGraph::qmlEdges() {
    QVariantList edges;
    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        QVariantMap vedge;
        vedge.insert("fromVertex", edge->fromVertex);
        vedge.insert("toVertex", edge->toVertex);
        vedge.insert("toInput", edge->toInput);
        edges.append(vedge);
    }
    return edges;
}

ModelGraph& ModelGraph::operator=(const ModelGraph &other) {
    m_vertices = other.m_vertices;
    m_edges = other.m_edges;
    return *this;
}

int ModelGraph::vertexCount(QQmlListProperty<VideoNode>* list) {
    return reinterpret_cast< ModelGraph* >(list->data)->vertexCount();
}

VideoNode* ModelGraph::vertexAt(QQmlListProperty<VideoNode>* list, int i) {
    return reinterpret_cast< ModelGraph* >(list->data)->vertexAt(i);
}
