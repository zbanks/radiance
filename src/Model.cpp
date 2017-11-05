#include "Model.h"
#include "VideoNode.h"
#include "main.h"
#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

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

Model::Model()
    : m_vnId(1) {
}

Model::~Model() {
}

void Model::setChains(QList<QSharedPointer<Chain>> chains) {
    m_chains = chains;
    for (int i=0; i<m_vertices.count(); i++) {
        m_vertices.at(i)->setChains(chains);
    }
    emit chainsChanged(chains);
}

QList<QSharedPointer<Chain>> Model::chains() {
    return m_chains;
}

QMap<QString, VideoNode *> Model::outputConnections() const {
    return m_outputConnections;
}

void Model::connectOutput(QString outputName, VideoNode *videoNode) {
    if (outputName.isEmpty()) return;
    if (videoNode == nullptr) {
        m_outputConnections.remove(outputName);
    } else if (m_vertices.contains(videoNode)) {
        m_outputConnections.insert(outputName, videoNode);
    } else {
        qWarning() << "Attempt to connect non-existant videoNode to output";
    }
}

void Model::prepareNode(VideoNode *videoNode) {
    videoNode->setChains(m_chains);
    videoNode->setId(m_vnId++);

    connect(videoNode, &VideoNode::inputCountChanged, this, &Model::flush);
    connect(videoNode, &VideoNode::message, this, &Model::onMessage);
    connect(videoNode, &VideoNode::warning, this, &Model::onWarning);
    connect(videoNode, &VideoNode::fatal, this, &Model::onFatal);
}

void Model::disownNode(VideoNode *videoNode) {
    auto outputNames = m_outputConnections.keys();
    for (int i=0; i<outputNames.count(); i++) {
        auto outputName = outputNames.at(i);
        if (m_outputConnections.value(outputName, nullptr) == videoNode) {
            m_outputConnections.remove(outputName);
        }
    }
    disconnect(videoNode, &VideoNode::inputCountChanged, this, &Model::flush);
    disconnect(videoNode, &VideoNode::message, this, &Model::onMessage);
    disconnect(videoNode, &VideoNode::warning, this, &Model::onWarning);
    disconnect(videoNode, &VideoNode::fatal, this, &Model::onFatal);
}

void Model::onMessage(QString str) {
    auto vn = qobject_cast<VideoNode *>(sender());
    emit message(vn, str);
}

void Model::onWarning(QString str) {
    auto vn = qobject_cast<VideoNode *>(sender());
    emit warning(vn, str);
}

void Model::onFatal(QString str) {
    auto vn = qobject_cast<VideoNode *>(sender());
    emit fatal(vn, str);
    removeVideoNode(vn);
    flush();
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
        prepareNode(videoNode);
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

    disownNode(videoNode);
    m_vertices.removeAll(videoNode);

    if (videoNode->parent() == this) {
        videoNode->deleteLater();
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

    for (int i=0; i<m_edgesForRendering.count(); i++) {
        out.fromVertex.append(map.value(m_edgesForRendering.at(i).fromVertex, -1));
        out.toVertex.append(map.value(m_edgesForRendering.at(i).toVertex, -1));
        out.toInput.append(m_edgesForRendering.at(i).toInput);
    }

    for (int i=0; i<m_verticesSortedForRendering.count(); i++) {
        out.vertices.append(m_verticesSortedForRendering.at(i)->createCopyForRendering());
    }

    auto o = m_outputConnectionsForRendering.keys();
    for (int i=0; i<o.count(); i++) {
        out.outputs.insert(o.at(i), m_outputConnectionsForRendering.value(o.at(i))->id());
    }
    return out;
}

void Model::copyBackRenderStates(QSharedPointer<Chain> chain, const ModelCopyForRendering *modelCopy) {
    QMutexLocker locker(&m_graphLock);
    QMap<int, QSharedPointer<VideoNode>> m;
    for (int i=0; i<modelCopy->vertices.count(); i++) {
        auto vnId = modelCopy->vertices.at(i)->id();
        if (vnId != 0) {
            m.insert(vnId, modelCopy->vertices.at(i));
        }
    }
    for (int i=0; i<m_vertices.count(); i++) {
        auto videoNodeCopy = m.value(m_vertices.at(i)->id());
        if (!videoNodeCopy.isNull()) {
            m_vertices[i]->copyBackRenderState(chain, videoNodeCopy);
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

QVariantMap Model::qmlOutputConnections() const {
    QVariantMap outputConnections;

    auto o = m_outputConnections.keys();
    for (int i=0; i<o.count(); i++) {
        outputConnections.insert(o.at(i), QVariant::fromValue(m_outputConnections.value(o.at(i))));
    }
    return outputConnections;
}

void Model::clear() {
    while (!m_vertices.empty()) {
        removeVideoNode(m_vertices[0]);
    }
}

void Model::flush() {
    QList<VideoNode *> verticesAdded;
    QList<VideoNode *> verticesRemoved;
    QList<Edge> edgesAdded;
    QList<Edge> edgesRemoved;
    QMap<QString, VideoNode *> outputsAdded;
    QMap<QString, VideoNode *> outputsRemoved;

    // Prune invalid edges
    QMutableListIterator<Edge> i(m_edges);
    while (i.hasNext()) {
        auto edgeCopy = i.next();
        if (edgeCopy.toInput >= edgeCopy.toVertex->inputCount()) {
            qDebug() << "Removing invalid edge to" << edgeCopy.toVertex << edgeCopy.toInput;
            i.remove();
        }
    }

    // Compute the changeset
    {
        auto v = QSet<VideoNode *>::fromList(m_vertices);
        auto v4r = QSet<VideoNode *>::fromList(m_verticesForRendering);
        // TODO Can't use QSet without implementing qHash
        //auto e = QSet<Edge>::fromList(m_edges);
        //auto e4r = QSet<Edge>::fromList(m_edgesForRendering);
        auto e = m_edges;
        auto e4r = m_edgesForRendering;

        auto o = m_outputConnections.keys();
        auto o4r = m_outputConnectionsForRendering.keys();

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
        for (int i=0; i<o.count(); i++) {
            if (m_outputConnectionsForRendering.value(o.at(i), nullptr) != m_outputConnections.value(o.at(i))) {
                outputsAdded.insert(o.at(i), m_outputConnections.value(o.at(i)));
            }
        }
        for (int i=0; i<o4r.count(); i++) {
            if (m_outputConnections.value(o4r.at(i), nullptr) != m_outputConnectionsForRendering.value(o4r.at(i))) {
                outputsRemoved.insert(o4r.at(i), m_outputConnectionsForRendering.value(o4r.at(i)));
            }
        }
    }

    // Swap
    {
        QMutexLocker locker(&m_graphLock);
        m_verticesForRendering = m_vertices;
        m_edgesForRendering = m_edges;
        m_verticesSortedForRendering = topoSort();
        m_outputConnectionsForRendering = m_outputConnections;
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

    QVariantMap outputsAddedVM;
    auto o_a = outputsAdded.keys();
    for (int i=0; i<o_a.count(); i++) outputsAddedVM.insert(o_a.at(i), QVariant::fromValue(outputsAdded.value(o_a.at(i))));

    QVariantMap outputsRemovedVM;
    auto o_r = outputsRemoved.keys();
    for (int i=0; i<o_r.count(); i++) outputsRemovedVM.insert(o_r.at(i), QVariant::fromValue(outputsRemoved.value(o_r.at(i))));

    emit graphChanged(verticesAddedVL, verticesRemovedVL, edgesAddedVL, edgesRemovedVL, outputsAddedVM, outputsRemovedVM);
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

QMap<int, GLuint> ModelCopyForRendering::render(QSharedPointer<Chain> chain) {
    //qDebug() << "RENDER!" << chain;

    // inputs is parallel to vertices
    // and contains the VideoNodes connected to the
    // corresponding vertex's inputs
    QVector<QVector<int>> inputs;

    auto & vao = chain->vao();
    if(!vao.isCreated())
        vao.create();
    // Create a list of -1's
    for (int i=0; i<vertices.count(); i++) {
        auto inputCount = vertices.at(i)->inputCount();
        inputs.append(QVector<int>(inputCount, -1));
    }

    Q_ASSERT(fromVertex.count() == toVertex.count());
    Q_ASSERT(fromVertex.count() == toInput.count());
    for (int i = 0; i < toVertex.count(); i++) {
        auto to = toVertex.at(i);
        if (to >= 0) {
            if (toInput.at(i) < inputs.at(to).count())
                inputs[to][toInput.at(i)] = fromVertex.at(i);
        }
    }

    QVector<GLuint> resultTextures(vertices.count(), 0);

    for (int i=0; i<vertices.count(); i++) {
        auto vertex = vertices.at(i);
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
        vao.bind();
        resultTextures[i] = vertex->paint(chain, inputTextures);
        //qDebug() << vertex << "wrote texture" << vertex->texture(chain);
    }

    QMap<int, GLuint> result;
    for (int i=0; i<resultTextures.count(); i++) {
        if (resultTextures.at(i) != 0 && vertices.at(i)->id() != 0) {
            result.insert(vertices.at(i)->id(), resultTextures.at(i));
        }
    }
    vao.release();
    return result;
}

/*
void Model::serialize(QTextStream *output) {
    for (auto vertex : m_vertices) {
        *output << "v " << vertex->id() << " " << nodeRegistry->serializeNode(vertex) << "\n";
    }
    for (auto edge : m_edges) {
        *output << "e " << edge.fromVertex->id() <<
                    " " << edge.toVertex->id() <<
                    " " << edge.toInput << "\n";
    }
}
*/

QJsonObject Model::serialize() {
    QJsonObject jsonOutput;

    QJsonObject jsonVertices;
    for (auto vertex : m_vertices) {
        jsonVertices[QString::number(vertex->id())] = vertex->serialize();
    }
    jsonOutput["vertices"] = jsonVertices;

    QJsonArray jsonEdges;
    for (auto edge : m_edges) {
        QJsonObject jsonEdge;
        jsonEdge["fromVertex"] = QString::number(edge.fromVertex->id());
        jsonEdge["toVertex"] = QString::number(edge.toVertex->id());
        jsonEdge["toInput"] = edge.toInput;
        jsonEdges.append(jsonEdge);
    }
    jsonOutput["edges"] = jsonEdges;

    QJsonObject jsonOutputConnections;
    for (auto outputName : m_outputConnections.keys()) {
        jsonOutputConnections[outputName] = QString::number(m_outputConnections[outputName]->id());
    }
    jsonOutput["outputConnections"] = jsonOutputConnections;

    return jsonOutput;
}

void Model::deserialize(const QJsonObject &data) {
    //TODO: needs error handling

    QMap<QString, VideoNode *> addedVertices;
    QJsonObject jsonVertices = data["vertices"].toObject();
    for (auto vertexName : jsonVertices.keys()) {
        VideoNode *vertex = createVideoNode(jsonVertices[vertexName].toString());
        qInfo() << vertex << jsonVertices[vertexName];
        addedVertices.insert(vertexName, vertex);
    }

    QJsonArray jsonEdges = data["edges"].toArray();
    for (auto _jsonEdge : jsonEdges) {
        QJsonObject jsonEdge = _jsonEdge.toObject();
        VideoNode *fromVertex = addedVertices.value(jsonEdge["fromVertex"].toString());
        VideoNode *toVertex = addedVertices.value(jsonEdge["toVertex"].toString());
        int toInput = jsonEdge["toInput"].toInt();
        addEdge(fromVertex, toVertex, toInput);
    }

    QJsonObject jsonOutputConnections = data["outputConnections"].toObject();
    for (auto outputName : jsonOutputConnections.keys()) {
        connectOutput(outputName, addedVertices.value(jsonOutputConnections[outputName].toString()));
    }
}

void Model::loadFile(QString filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open file for reading:" << filename;
        return;
    }
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    deserialize(doc.object());
    flush();
}

void Model::saveFile(QString filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open file for writing:" << filename;
        return;
    }
    QJsonDocument doc(serialize());
    file.write(doc.toJson());
}
