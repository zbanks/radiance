#include "Model.h"
#include "EffectNode.h"
#include "main.h"

QVariantMap Edge::toVariantMap() const {
    QVariantMap result;
    result.insert("fromVertex", QVariant::fromValue(fromVertex));
    result.insert("toVertex", QVariant::fromValue(toVertex));
    result.insert("toInput", toInput);
    return result;
}

Model::Model() {
}

Model::~Model() {
}

void Model::addVideoNode(VideoNode *videoNode) {
    if (!m_vertices.contains(videoNode)) {
        {
            QMutexLocker locker(&m_graphLock);
            m_vertices.append(videoNode);
            m_verticesSortedForRendering = topoSort();
        }
        emit videoNodeAdded(videoNode);
    }
}

void Model::removeVideoNode(VideoNode *videoNode) {
    QList<Edge> removed;

    int count;
    {
        QMutexLocker locker(&m_graphLock);

        QMutableListIterator<Edge> i(m_edges);
        while (i.hasNext()) {
            auto edgeCopy = i.next();
            if (edgeCopy.fromVertex == videoNode || edgeCopy.toVertex == videoNode) {
                i.remove();
                removed.append(edgeCopy);
            }
        }

        count = m_vertices.removeAll(videoNode);
    }

    if (removed.count() > 0 || count > 0) {
        m_verticesSortedForRendering = topoSort();
        for (auto edge = removed.begin(); edge != removed.end(); edge++) {
            emit edgeRemoved(edge->fromVertex, edge->toVertex, edge->toInput);
        }
        if (count > 0) {
            emit videoNodeRemoved(videoNode);
        }
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

    Edge newEdge = {
        .fromVertex = fromVertex,
        .toVertex = toVertex,
        .toInput = toInput,
    };
    {
        QMutexLocker locker(&m_graphLock);

        QMutableListIterator<Edge> i(m_edges);
        while (i.hasNext()) {
            Edge edgeCopy = i.next();
            if (edgeCopy.toVertex == toVertex && edgeCopy.toInput == toInput) {
                if (edgeCopy.fromVertex == fromVertex)
                    return;
                qInfo() << "erasing" << edgeCopy.fromVertex << edgeCopy.toVertex << edgeCopy.toInput;
                i.remove();
                removed.append(edgeCopy);
            }
        }

        m_edges.append(newEdge);
        QVector<VideoNode *> sortedVertices = topoSort();
        if(sortedVertices.count() < m_vertices.count()) {
            // Roll back changes if a cycle was detected
            m_edges = edgesOld;
            return;
        }
        m_verticesSortedForRendering = sortedVertices;
    }
    for (auto edge = removed.begin(); edge != removed.end(); edge++) {
        emit edgeRemoved(edge->fromVertex, edge->toVertex, edge->toInput);
    }
    emit edgeAdded(newEdge.fromVertex, newEdge.toVertex, newEdge.toInput);
}

void Model::removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    Q_ASSERT(fromVertex != nullptr);
    Q_ASSERT(toVertex != nullptr);
    Q_ASSERT(toInput >= 0);
    Q_ASSERT(m_vertices.contains(fromVertex));
    Q_ASSERT(m_vertices.contains(toVertex));


    Edge edgeCopy;
    bool removed = false;
    {
        QMutexLocker locker(&m_graphLock);
        QMutableListIterator<Edge> i(m_edges);
        while (i.hasNext()) {
            edgeCopy = i.next();
            if (edgeCopy.fromVertex == fromVertex
             && edgeCopy.toVertex == toVertex
             && edgeCopy.toInput == toInput) {
                i.remove();
                removed = true;
                break;
            }
        }
        if (removed) {
            m_verticesSortedForRendering = topoSort();
        }
    }
    if (removed) {
        emit edgeRemoved(edgeCopy.fromVertex, edgeCopy.toVertex, edgeCopy.toInput);
    }
}

ModelCopyForRendering Model::createCopyForRendering() {
    QMutexLocker locker(&m_graphLock);
    ModelCopyForRendering out;

    // Create a map from VideoNodes to indices
    QMap<VideoNode *, int> map;
    for (int i=0; i<m_verticesSortedForRendering.count(); i++) {
        map.insert(m_verticesSortedForRendering.at(i), i);
    }

    for(int i=0; i<m_edges.count(); i++) {
        out.fromVertex.append(map.value(m_edges.at(i).fromVertex, -1));
        out.toVertex.append(map.value(m_edges.at(i).toVertex, -1));
        out.toInput.append(m_edges.at(i).toInput);
    }

    out.origVertices = m_verticesSortedForRendering;
    QVector<QSharedPointer<VideoNode>> vertices;
    for(int i=0; i<out.origVertices.count(); i++) {
        vertices.append(out.origVertices.at(i)->createCopyForRendering());
    }
    out.vertices = vertices;
    return out;
}

void Model::copyBackRenderStates(int chain, QVector<VideoNode *> origVertices, QVector<QSharedPointer<VideoNode>> renderedVertices) {
    Q_ASSERT(origVertices.count() == renderedVertices.count());
    {
        QMutexLocker locker(&m_graphLock);
        for(int i=0; i<origVertices.count(); i++) {
            // If the VideoNode is still in the DAG,
            // Javascript shouldn't have deleted it
            if(m_vertices.contains(origVertices.at(i))) {
                origVertices[i]->copyBackRenderState(chain, renderedVertices.at(i));
            } else {
                //qDebug() << "Node was deleted during rendering";
            }
        }
    }
}

QVector<VideoNode *> Model::topoSort() {
    // Kahn's algorithm from Wikipedia

    auto edges = m_edges;

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
        for (auto e = edges.begin(); e != edges.end();) {
            // Remove edges originating from the node we just took
            // See https://stackoverflow.com/questions/16269696/erasing-while-iterating-an-stdlist
            // about erasing while using iterators
            if (e->fromVertex == n) {
                newStartNodes.insert(e->toVertex);
                e = edges.erase(e);
                // Any node pointed to by one of these deleted edges
                // is a potential new start node
            } else {
                e++;
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

QList<VideoNode *> Model::vertices() const {
    return m_vertices;
}

QList<Edge> Model::edges() const {
    return m_edges;
}

QQmlListProperty<VideoNode> Model::qmlVertices() {
    return QQmlListProperty<VideoNode>(this, this, &Model::vertexCount, &Model::vertexAt);
}

int Model::vertexCount() const {
    return m_vertices.count();
}

VideoNode *Model::vertexAt(int index) const {
    return m_vertices.at(index);
}

QVariantList Model::qmlEdges() const {
    QVariantList edges;

    QMap<VideoNode *, int> map;
    int i = 0;
    for (int i=0; i<m_vertices.count(); i++) {
        map.insert(m_vertices.at(i), i);
    }

    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        QVariantMap vedge;
        vedge.insert("fromVertex", map.value(edge->fromVertex, -1));
        vedge.insert("toVertex", map.value(edge->toVertex, -1));
        vedge.insert("toInput", edge->toInput);
        edges.append(vedge);
    }
    return edges;
}

QList<VideoNode *> Model::ancestors(VideoNode *node) {
    QSet<VideoNode *> ancestorSet;
    QList<VideoNode *> nodeStack;
    nodeStack.append(node);

    while (!nodeStack.isEmpty()) {
        VideoNode * n = nodeStack.takeLast();
        for (auto e : m_edges) {
            if (e.toVertex != n)
                continue;

            VideoNode * newNode = e.fromVertex;
            Q_ASSERT(newNode != node); // cycles are bad
            if (ancestorSet.contains(newNode))
                continue;

            ancestorSet.insert(newNode);
            nodeStack.append(newNode);
        }
    }

    return ancestorSet.values();
}

bool Model::isAncestor(VideoNode *parent, VideoNode *child) {
    // TODO: This is obviously not optimal
    return ancestors(child).contains(parent);
}

int Model::vertexCount(QQmlListProperty<VideoNode>* list) {
    return reinterpret_cast< Model* >(list->data)->vertexCount();
}

VideoNode* Model::vertexAt(QQmlListProperty<VideoNode>* list, int i) {
    return reinterpret_cast< Model* >(list->data)->vertexAt(i);
}

