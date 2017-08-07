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
        regenerateGraph(topoSort());
        emit videoNodeAdded(videoNode);
        emitGraphChanged();
    }
}

void Model::removeVideoNode(VideoNode *videoNode) {
    QList<Edge> removed;

    QMutableListIterator<Edge> i(m_edges);
    while (i.hasNext()) {
        auto edgeCopy = i.next();
        if (edgeCopy.fromVertex == videoNode || edgeCopy.toVertex == videoNode) {
            i.remove();
            removed.append(edgeCopy);
        }
    }

    int count = m_vertices.removeAll(videoNode);

    if (removed.count() > 0 || count > 0) {
        if (count > 0) {
            // Wait until there are truly no more refs
            // before emitting deleted
            connect(videoNode, &VideoNode::noMoreRef, this, &Model::videoNodeRemoved, Qt::QueuedConnection);
        }
        regenerateGraph(topoSort());
        for (auto edge = removed.begin(); edge != removed.end(); edge++) {
            emit edgeRemoved(edge->fromVertex, edge->toVertex, edge->toInput);
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

    QList<Edge> edgesOld;

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
    QVector<VideoNode *> sortedVertices = topoSort();
    if(sortedVertices.count() < m_vertices.count()) {
        // Roll back changes if a cycle was detected
        m_edges = edgesOld;
        return;
    }
    regenerateGraph(sortedVertices);
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
            regenerateGraph(topoSort());
            emit edgeRemoved(edgeCopy.fromVertex, edgeCopy.toVertex, edgeCopy.toInput);
            emitGraphChanged();
            return;
        }
    }
}

void Model::regenerateGraph(QVector<VideoNode *> sortedVertices) {
    auto tmpGraph = ModelGraph(sortedVertices, m_edges);
    {
        QMutexLocker locker(&m_graphLock);
        tmpGraph.ref();
        m_graph.deref();
        m_graph = tmpGraph;
    }
}

void Model::emitGraphChanged() {
    emit graphChanged(m_graph);
    emit qmlGraphChanged();
}

ModelGraph Model::graph() {
    QMutexLocker locker(&m_graphLock);
    return m_graph;
}

ModelGraph Model::graphRef() {
    QMutexLocker locker(&m_graphLock);
    m_graph.ref();
    return m_graph;
}

ModelGraph *Model::qmlGraph() {
    QMutexLocker locker(&m_graphLock);
    ModelGraph *mg = new ModelGraph(m_graph);
    QQmlEngine::setObjectOwnership(mg, QQmlEngine::JavaScriptOwnership);
    return mg;
}

QVector<VideoNode *> Model::topoSort() {
    // Kahn's algorithm from Wikipedia

    QList<Edge> edges = m_edges;

    QVector<VideoNode *> l;
    QList<VideoNode *> s;
    {
        QSet<VideoNode *> sSet;
        // Populate s, the start nodes
        for (auto v = m_vertices.begin(); v != m_vertices.end(); v++) {
            sSet.insert(*v);
        }
        for (auto e = edges.begin(); e != edges.end(); e++) {
            sSet.remove(e->toVertex);
        }
        s = sSet.values();
    }

    while(!s.empty()) {
        // Put a start node into the sorted list
        auto n = s.takeFirst();
        l.append(n);

        QSet<VideoNode *> newStartNodes; // Potential new start nodes
        for (auto e = edges.begin(); e != edges.end(); e++) {
            // Remove edges originating from the node we just took
            if (e->fromVertex == n) {
                newStartNodes.insert(e->toVertex);
                edges.erase(e);
                // Any node pointed to by one of these deleted edges
                // is a potential new start node
            }
        }
        // Prune the potential new start nodes down to just the ones
        // with no edges
        for (auto e = edges.begin(); e != edges.end(); e++) {
            newStartNodes.remove(e->toVertex);
        }
        // Add them to s
        for (auto newStartNode = newStartNodes.begin(); newStartNode != newStartNodes.end(); newStartNode++) {
            s.append(*newStartNode);
        }
    }

    // If there are edges left, they are part of cycles
    if(!edges.empty()) {
        qDebug() << "Cycle detected!";
    }
    return l;
}

// ModelGraph methods

ModelGraph::ModelGraph() {
}

ModelGraph::ModelGraph(QVector<VideoNode *> vertices, QList<Edge> edges)
    : m_vertices(vertices) {
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

ModelGraph::~ModelGraph() {
}

// This reference counting bullshit
// is sort of bullshit. It would be
// much better if it could live in a
// constructor / destructor.
void ModelGraph::ref() {
    for (int i=0; i<m_vertices.count(); i++) {
        m_vertices[i]->ref();
    }
}

void ModelGraph::deref() {
    for (int i=0; i<m_vertices.count(); i++) {
        m_vertices[i]->deRef();
    }
}

QVector<VideoNode *> ModelGraph::vertices() const {
    return m_vertices;
}

QVector<ModelGraphEdge> ModelGraph::edges() const {
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

QVariantList ModelGraph::qmlEdges() const {
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
