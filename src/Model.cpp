#include "Model.h"
#include "Context.h"
#include "Paths.h"
#include "Registry.h"
#include "VideoNode.h"
#include "QmlSharedPointer.h"
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtGlobal>

static QString vnp(VideoNodeSP *videoNode) {
    if (videoNode) return QString("%1(%2)").arg((*videoNode)->metaObject()->className()).arg((qintptr)videoNode->data());
    return "null";
}

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

void Model::addChain(QSharedPointer<Chain> chain) {
    if (!m_chains.contains(chain)) {
        m_chains.append(chain);
        emit chainsChanged(m_chains);
    }
}

void Model::removeChain(QSharedPointer<Chain> chain) {
    if (m_chains.contains(chain)) {
        m_chains.removeAll(chain);
        emit chainsChanged(m_chains);
    }
}

QList<QSharedPointer<Chain>> Model::chains() {
    return m_chains;
}

void Model::prepareNode(VideoNodeSP *videoNode) {
    if(!videoNode)
        return;

    videoNode->setParent(this);

    (*videoNode)->setLastModel(QWeakPointer<Model>(qSharedPointerCast<Model>(sharedFromThis())));
    (*videoNode)->setChains(m_chains);

    connect(videoNode->data(), &VideoNode::inputCountChanged, this, &Model::flush);
    connect(videoNode->data(), &VideoNode::message, this, &Model::onMessage);
    connect(videoNode->data(), &VideoNode::warning, this, &Model::onWarning);
    connect(videoNode->data(), &VideoNode::error, this, &Model::onError);
    connect(this, &Model::chainsChanged, videoNode->data(), &VideoNode::setChains);

    // See if this VideoNodeSP requests any chains
    auto requestedChains = (*videoNode)->requestedChains();
    for (auto c = requestedChains.begin(); c != requestedChains.end(); c++) {
        if (!m_chains.contains(*c)) {
            addChain(*c);
        }
    }
    // and be notified of changes
    connect(videoNode->data(), &VideoNode::requestedChainAdded, this, &Model::addChain);
    connect(videoNode->data(), &VideoNode::requestedChainRemoved, this, &Model::removeChain);
}

void Model::disownNode(VideoNodeSP *videoNode) {
    if(!videoNode)
        return;

    disconnect(this, &Model::chainsChanged, videoNode->data(), &VideoNode::setChains);
    disconnect(videoNode->data(), &VideoNode::requestedChainAdded, this, &Model::addChain);
    disconnect(videoNode->data(), &VideoNode::requestedChainRemoved, this, &Model::removeChain);

    disconnect(videoNode->data(), &VideoNode::inputCountChanged, this, &Model::flush);
    disconnect(videoNode->data(), &VideoNode::message, this, &Model::onMessage);
    disconnect(videoNode->data(), &VideoNode::warning, this, &Model::onWarning);
    disconnect(videoNode->data(), &VideoNode::error, this, &Model::onError);

    auto requestedChains = (*videoNode)->requestedChains();
    auto chains = m_chains;
    for (auto c = requestedChains.begin(); c != requestedChains.end(); c++) {
        m_chains.removeAll(*c);
    }

    if (chains != m_chains) {
        emit chainsChanged(m_chains);
    }
}

VideoNodeSP *Model::lookupSender() {
    auto vn_data = qobject_cast<VideoNode *>(sender());
    for (auto vertex = m_vertices.begin(); vertex != m_vertices.end(); vertex++) {
        if ((*vertex)->data() == vn_data) {
            return *vertex;
        }
    }
    return nullptr;
}

void Model::onMessage(QString str) {
    auto vn = lookupSender();
    emit message(vn, str);
}

void Model::onWarning(QString str) {
    auto vn = lookupSender();
    emit warning(vn, str);
}

void Model::onError(QString str) {
    auto vn = lookupSender();
    emit error(vn, str);
}

void Model::addVideoNode(VideoNodeSP *videoNode) {
    if(!videoNode) {
        return;
    }
    if (!m_vertices.contains(videoNode)) {
        prepareNode(videoNode);
        m_vertices.append(videoNode);
    }
}

void Model::removeVideoNode(VideoNodeSP *videoNode) {
    if (!videoNode) {
        return;
    }
    if (!m_vertices.contains(videoNode)) {
        qWarning() << QString("Attempted to remove %1 which is not in the model").arg(vnp(videoNode));
        return;
    }

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

void Model::addEdge(VideoNodeSP *fromVertex, VideoNodeSP *toVertex, int toInput) {
    if (fromVertex == nullptr
     || toVertex == nullptr
     || toInput < 0
     || !m_vertices.contains(fromVertex)
     || !m_vertices.contains(toVertex)) {
        qWarning() << QString("Bad edge: %1 to %2 input %3").arg(vnp(fromVertex)).arg(vnp(toVertex)).arg(toInput);
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
    auto sortedVertices = topoSort();
    if(sortedVertices.count() < m_vertices.count()) {
        // Roll back changes if a cycle was detected
        m_edges = edgesOld;
        qWarning() << QString("Not adding edge because it would create a cycle: %1 to %2 input %3").arg(vnp(fromVertex)).arg(vnp(toVertex)).arg(toInput);
        return;
    }
}

void Model::removeEdge(VideoNodeSP *fromVertex, VideoNodeSP *toVertex, int toInput) {
    if (fromVertex == nullptr
     || toVertex == nullptr
     || toInput < 0
     || !m_vertices.contains(fromVertex)
     || !m_vertices.contains(toVertex)) {
        qWarning() << QString("Bad edge: %1 to %2 input %3").arg(vnp(fromVertex)).arg(vnp(toVertex)).arg(toInput);
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
    QMap<QSharedPointer<VideoNode>, int> map;
    for (int i=0; i<m_verticesSortedForRendering.count(); i++) {
        map.insert(m_verticesSortedForRendering.at(i), i);
    }

    for (int i=0; i<m_edgesForRendering.count(); i++) {
        auto fromVertex = qSharedPointerCast<VideoNode>(*m_edgesForRendering.at(i).fromVertex);
        auto toVertex = qSharedPointerCast<VideoNode>(*m_edgesForRendering.at(i).toVertex);

        out.fromVertex.append(map.value(fromVertex, -1));
        out.toVertex.append(map.value(toVertex, -1));
        out.toInput.append(m_edgesForRendering.at(i).toInput);
    }

    for (int i=0; i<m_verticesSortedForRendering.count(); i++) {
        out.vertices.append(m_verticesSortedForRendering.at(i));
    }

    return out;
}

QVector<QSharedPointer<VideoNode>> Model::topoSort() {
    // Kahn's algorithm from Wikipedia

    auto edges = m_edges;

    QVector<QSharedPointer<VideoNode>> l;
    QList<QSharedPointer<VideoNode>> s;
    {
        QSet<QSharedPointer<VideoNode>> sSet;
        // Populate s, the start nodes
        for (auto v = m_vertices.begin(); v != m_vertices.end(); v++) {
            sSet.insert(qSharedPointerCast<VideoNode>(**v));
        }
        for (auto e = edges.begin(); e != edges.end(); e++) {
            sSet.remove(qSharedPointerCast<VideoNode>(*e->toVertex));
        }
        s = sSet.values();
    }

    while(!s.empty()) {
        // Put a start node into the sorted list
        auto n = s.takeFirst();
        l.append(n);

        QSet<QSharedPointer<VideoNode>> newStartNodes; // Potential new start nodes
        for (auto e = edges.begin(); e != edges.end();) {
            // Remove edges originating from the node we just took
            // See https://stackoverflow.com/questions/16269696/erasing-while-iterating-an-stdlist
            // about erasing while using iterators
            if (qSharedPointerCast<VideoNode>(*e->fromVertex) == n) {
                newStartNodes.insert(qSharedPointerCast<VideoNode>(*e->toVertex));
                e = edges.erase(e);
                // Any node pointed to by one of these deleted edges
                // is a potential new start node
            } else {
                e++;
            }
        }
        // Prune the potential new start nodes down to just the ones
        for (auto e = edges.begin(); e != edges.end(); e++) {
            newStartNodes.remove(qSharedPointerCast<VideoNode>(*e->toVertex));
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

QList<VideoNodeSP *> Model::vertices() const {
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

void Model::clear() {
    while (!m_vertices.empty()) {
        removeVideoNode(m_vertices[0]);
    }
}

void Model::flush() {
    QList<VideoNodeSP *> verticesAdded;
    QList<VideoNodeSP *> verticesRemoved;
    QList<Edge> edgesAdded;
    QList<Edge> edgesRemoved;

    // Prune invalid edges
    QMutableListIterator<Edge> i(m_edges);
    while (i.hasNext()) {
        auto edgeCopy = i.next();
        if (edgeCopy.toInput >= (*edgeCopy.toVertex)->inputCount()) {
            qDebug() << QString("Removing invalid edge to %1 input %2").arg(vnp(edgeCopy.toVertex)).arg(edgeCopy.toInput);
            i.remove();
        }
    }

    // Compute the changeset
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        auto v = QSet<VideoNodeSP *>::fromList(m_vertices);
        auto v4r = QSet<VideoNodeSP *>::fromList(m_verticesForRendering);
#else
        auto v = QSet<VideoNodeSP *>(m_vertices.begin(), m_vertices.end());
        auto v4r = QSet<VideoNodeSP *>(m_verticesForRendering.begin(), m_verticesForRendering.end());
#endif
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

QList<VideoNodeSP *> Model::ancestors(VideoNodeSP *node) const {
    QSet<VideoNodeSP *> ancestorSet;
    QList<VideoNodeSP *> nodeStack;
    nodeStack.append(node);

    while (!nodeStack.isEmpty()) {
        VideoNodeSP * n = nodeStack.takeLast();
        for (auto e : m_edges) {
            if (e.toVertex != n)
                continue;

            VideoNodeSP * newNode = e.fromVertex;
            Q_ASSERT(newNode != node); // cycles are bad
            if (ancestorSet.contains(newNode))
                continue;

            ancestorSet.insert(newNode);
            nodeStack.append(newNode);
        }
    }

    return ancestorSet.values();
}

bool Model::isAncestor(VideoNodeSP *parent, VideoNodeSP *child) const {
    // TODO: This is obviously not optimal
    return ancestors(child).contains(parent);
}

QVariantList Model::qmlAncestors(VideoNodeSP *vn) const {
    QList<VideoNodeSP *> nodes = ancestors(vn);
    QVariantList output;

    for (auto node = nodes.begin(); node != nodes.end(); node++) {
        output.append(QVariant::fromValue(*node));
    }
    return output;
}


QList<VideoNodeSP *> Model::descendants(VideoNodeSP *node) const {
    QSet<VideoNodeSP *> descendantSet;
    QList<VideoNodeSP *> nodeStack;
    nodeStack.append(node);

    while (!nodeStack.isEmpty()) {
        VideoNodeSP * n = nodeStack.takeLast();
        for (auto e : m_edges) {
            if (e.fromVertex != n)
                continue;

            VideoNodeSP * newNode = e.toVertex;
            Q_ASSERT(newNode != node); // cycles are bad
            if (descendantSet.contains(newNode))
                continue;

            descendantSet.insert(newNode);
            nodeStack.append(newNode);
        }
    }

    return descendantSet.values();
}

bool Model::isDescendant(VideoNodeSP *parent, VideoNodeSP *child) const {
    // TODO: This is obviously not optimal
    return descendants(child).contains(parent);
}

QVariantList Model::qmlDescendants(VideoNodeSP *vn) const {
    QList<VideoNodeSP *> nodes = descendants(vn);
    QVariantList output;

    for (auto node = nodes.begin(); node != nodes.end(); node++) {
        output.append(QVariant::fromValue(*node));
    }
    return output;
}

QMap<QSharedPointer<VideoNode>, GLuint> ModelCopyForRendering::render(QSharedPointer<Chain> chain) {
    // inputs is parallel to vertices
    // and contains the VideoNodes connected to the
    // corresponding vertex's inputs
    QVector<QVector<int>> inputs;
    auto vao = chain->vao();

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
        vao->bind();
        resultTextures[i] = vertex->paint(chain, inputTextures);
    }

    QMap<QSharedPointer<VideoNode>, GLuint> result;
    for (int i=0; i<resultTextures.count(); i++) {
        if (resultTextures.at(i) != 0) {
            result.insert(vertices.at(i), resultTextures.at(i));
        }
    }
    vao->release();
    return result;
}

QJsonObject Model::serialize() {
    QJsonObject jsonOutput;

    // Create a map from VideoNodes to indices
    QMap<VideoNodeSP *, int> map;
    QJsonArray jsonVertices;
    for (int i=0; i<m_vertices.count(); i++) {
        map.insert(m_vertices.at(i), i);
        jsonVertices.append((*m_vertices.at(i))->serialize());
    }

    jsonOutput["vertices"] = jsonVertices;

    QJsonArray jsonEdges;
    for (auto edge : m_edges) {
        QJsonObject jsonEdge;
        jsonEdge["fromVertex"] = map.value(edge.fromVertex, -1);
        jsonEdge["toVertex"] = map.value(edge.toVertex, -1);
        jsonEdge["toInput"] = edge.toInput;
        jsonEdges.append(jsonEdge);
    }
    jsonOutput["edges"] = jsonEdges;

    return jsonOutput;
}

void Model::deserialize(Context *context, Registry *registry, const QJsonObject &data) {
    //TODO: needs error handling

    QList<VideoNodeSP *> addedVertices;
    QJsonArray jsonVertices = data["vertices"].toArray();
    for (auto _jsonVertex : jsonVertices) {
        QJsonObject jsonVertex = _jsonVertex.toObject();
        VideoNodeSP *vertex = registry->deserialize(context, jsonVertex);
        if (vertex != nullptr) {
            addVideoNode(vertex);
        }
        addedVertices.append(vertex);
    }

    QJsonArray jsonEdges = data["edges"].toArray();
    for (auto _jsonEdge : jsonEdges) {
        QJsonObject jsonEdge = _jsonEdge.toObject();
        VideoNodeSP *fromVertex = addedVertices.at(jsonEdge["fromVertex"].toInt());
        VideoNodeSP *toVertex = addedVertices.at(jsonEdge["toVertex"].toInt());
        int toInput = jsonEdge["toInput"].toInt();
        addEdge(fromVertex, toVertex, toInput);
    }
}

void Model::loadDefault(Context *context, Registry *registry) {
    auto fn = Paths::userConfig() + "/" + "model.json";
    if (!QFileInfo(fn).exists()) {
        fn = Paths::systemConfig() + "/" + "gui_default.json";
    }
    load(context, registry, fn);
}

void Model::saveDefault() {
    auto fn = Paths::ensureUserConfig("model.json");
    save(fn);
}

void Model::load(Context *context, Registry *registry, QString filename) {
    filename = Paths::expandLibraryPath(filename);
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open file for reading:" << filename;
        return;
    }
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    deserialize(context, registry, doc.object());
    flush();
}

void Model::save(QString filename) {
    filename = Paths::ensureUserLibrary(filename);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open file for writing:" << filename;
        return;
    }
    QJsonDocument doc(serialize());
    file.write(doc.toJson());
}

ModelCopyForRendering Model::createCopyForRendering(QWeakPointer<Model> model) {
    auto strong_ptr = model.toStrongRef();
    if (strong_ptr.isNull()) return ModelCopyForRendering();
    return strong_ptr->createCopyForRendering();
}
