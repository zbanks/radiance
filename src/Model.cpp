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

bool Edge::operator==(const Edge &other) const {
    return fromVertex == other.fromVertex
        && toVertex == other.toVertex
        && toInput == other.toInput;
}

Model::Model() {
}

Model::~Model() {
}

VideoNode *Model::createVideoNode(const QString &name) {
    VideoNode *videoNode = nodeRegistry->createNode(name);
    if (!videoNode) {
        qInfo() << "Failed to create videoNode:" << name;
        return nullptr;
    }

    videoNode->setParent(this);
    addVideoNode(videoNode);
    return videoNode;
}

void Model::addVideoNode(VideoNode *videoNode) {
    if (!m_vertices.contains(videoNode)) {
        m_vertices.append(videoNode);
    }
}

void Model::removeVideoNode(VideoNode *videoNode) {
    QList<Edge> removed;

    QMutableListIterator<Edge> i(m_edges);
    while (i.hasNext()) {
        auto edgeCopy = i.next();
        if (edgeCopy.fromVertex == videoNode || edgeCopy.toVertex == videoNode) {
            i.remove();
        }
    }

    m_vertices.removeAll(videoNode);

    if (videoNode->parent() == this) {
        videoNode->deleteLater(); // Not sure if this is required
    }
}

void Model::addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    if (fromVertex == nullptr
     || toVertex == nullptr
     || toInput < 0
     || !m_vertices.contains(fromVertex)
     || !m_vertices.contains(toVertex)) {
        qWarning() << "Bad edge:" << fromVertex << toVertex << toInput;
        return;
    }

    QList<Edge> removed;

    QList<Edge> edgesOld;

    Edge newEdge = {
        .fromVertex = fromVertex,
        .toVertex = toVertex,
        .toInput = toInput,
    };
    QMutableListIterator<Edge> i(m_edges);
    while (i.hasNext()) {
        Edge edgeCopy = i.next();
        if (edgeCopy.toVertex == toVertex && edgeCopy.toInput == toInput) {
            if (edgeCopy.fromVertex == fromVertex)
                return;
            qInfo() << "Erasing" << edgeCopy.fromVertex << edgeCopy.toVertex << edgeCopy.toInput;
            i.remove();
        }
    }

    m_edges.append(newEdge);
    QVector<VideoNode *> sortedVertices = topoSort();
    if(sortedVertices.count() < m_vertices.count()) {
        // Roll back changes if a cycle was detected
        m_edges = edgesOld;
        qWarning() << "Not adding edge because it would create a cycle: " << fromVertex << toVertex << toInput;
        return;
    }
}

void Model::removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    if (fromVertex == nullptr
     || toVertex == nullptr
     || toInput < 0
     || !m_vertices.contains(fromVertex)
     || !m_vertices.contains(toVertex)) {
        qWarning() << "Bad edge:" << fromVertex << toVertex << toInput;
        return;
    }

    Edge edgeCopy;
    QMutableListIterator<Edge> i(m_edges);
    while (i.hasNext()) {
        edgeCopy = i.next();
        if (edgeCopy.fromVertex == fromVertex
         && edgeCopy.toVertex == toVertex
         && edgeCopy.toInput == toInput) {
            i.remove();
            break;
        }
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

    for(int i=0; i<m_edgesForRendering.count(); i++) {
        out.fromVertex.append(map.value(m_edgesForRendering.at(i).fromVertex, -1));
        out.toVertex.append(map.value(m_edgesForRendering.at(i).toVertex, -1));
        out.toInput.append(m_edgesForRendering.at(i).toInput);
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

QVariantList Model::qmlVertices() const {
    QVariantList vertices;

    for (auto vertex = m_vertices.begin(); vertex != m_vertices.end(); vertex++) {
        vertices.append(QVariant::fromValue(*vertex));
    }
    return vertices;
}

QVariantList Model::qmlEdges() const {
    QVariantList edges;

    for (auto edge = m_edges.begin(); edge != m_edges.end(); edge++) {
        QVariantMap vedge;
        vedge.insert("fromVertex", QVariant::fromValue(edge->fromVertex));
        vedge.insert("toVertex", QVariant::fromValue(edge->toVertex));
        vedge.insert("toInput", edge->toInput);
        edges.append(vedge);
    }
    return edges;
}

void Model::flush() {
    QList<VideoNode *> verticesAdded;
    QList<VideoNode *> verticesRemoved;
    QList<Edge> edgesAdded;
    QList<Edge> edgesRemoved;

    // Compute the changeset
    {
        auto v = QSet<VideoNode *>::fromList(m_vertices);
        auto v4r = QSet<VideoNode *>::fromList(m_verticesForRendering);
        // TODO Can't use QSet without implementing qHash
        //auto e = QSet<Edge>::fromList(m_edges);
        //auto e4r = QSet<Edge>::fromList(m_edgesForRendering);
        auto e = m_edges;
        auto e4r = m_edgesForRendering;

        for (int i=0; i<m_vertices.count(); i++) {
            if (!v4r.contains(m_vertices.at(i))) verticesAdded.append(m_vertices.at(i));
        }
        for (int i=0; i<m_verticesForRendering.count(); i++) {
            if (!v.contains(m_verticesForRendering.at(i))) verticesRemoved.append(m_verticesForRendering.at(i));
        }
        for (int i=0; i<m_edges.count(); i++) {
            if (!e4r.contains(m_edges.at(i))) edgesAdded.append(m_edges.at(i));
        }
        for (int i=0; i<m_edgesForRendering.count(); i++) {
            if (!e.contains(m_edgesForRendering.at(i))) edgesRemoved.append(m_edgesForRendering.at(i));
        }
    }

    // Swap
    {
        QMutexLocker locker(&m_graphLock);
        m_verticesForRendering = m_vertices;
        m_edgesForRendering = m_edges;
        m_verticesSortedForRendering = topoSort();
    }

    // Convert the changeset to VariantLists for QML
    QVariantList verticesAddedVL;
    for (int i=0; i<verticesAdded.count(); i++) verticesAddedVL.append(QVariant::fromValue(verticesAdded.at(i)));

    QVariantList verticesRemovedVL;
    for (int i=0; i<verticesRemoved.count(); i++) verticesRemovedVL.append(QVariant::fromValue(verticesRemoved.at(i)));

    QVariantList edgesAddedVL;
    for (int i=0; i<edgesAdded.count(); i++) edgesAddedVL.append(edgesAdded.at(i).toVariantMap());

    QVariantList edgesRemovedVL;
    for (int i=0; i<edgesRemoved.count(); i++) edgesRemovedVL.append(edgesRemoved.at(i).toVariantMap());

    emit graphChanged(verticesAddedVL, verticesRemovedVL, edgesAddedVL, edgesRemovedVL);
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
