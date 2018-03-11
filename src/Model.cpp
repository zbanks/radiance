#include "Model.h"
#include "Context.h"
#include "Paths.h"
#include "Registry.h"
#include "VideoNode.h"
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

static QString vnp(VideoNode *videoNode) {
    if (videoNode) return *videoNode;
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

Model::Model()
    : d_ptr(new ModelPrivate())
{
}

Model::Model(const Model &other)
    : d_ptr(other.d_ptr)
{
}

Model::Model(QSharedPointer<ModelPrivate> other_ptr)
    : d_ptr(other_ptr)
{
}

void Model::addChain(Chain chain) {
    if (!d_ptr->m_chains.contains(chain)) {
        d_ptr->m_chains.append(chain);
        emit chainsChanged(d_ptr->m_chains);
    }
}

void Model::removeChain(Chain chain) {
    if (d_ptr->m_chains.contains(chain)) {
        d_ptr->m_chains.removeAll(chain);
        emit chainsChanged(d_ptr->m_chains);
    }
}

QList<Chain> Model::chains() {
    return d_ptr->m_chains;
}

void Model::prepareNode(VideoNode *videoNode) {
    if(!videoNode)
        return;

    // Giving a QSP a parent... hmm...
    if(videoNode->parent() != this)
        videoNode->setParent(this);

    videoNode->setLastModel(WeakModel(*this));

    videoNode->setChains(d_ptr->m_chains);

    connect(videoNode, &VideoNode::inputCountChanged, this, &Model::flush);
    connect(videoNode, &VideoNode::message, this, &Model::onMessage);
    connect(videoNode, &VideoNode::warning, this, &Model::onWarning);
    connect(videoNode, &VideoNode::fatal, this, &Model::onFatal);
    connect(this, &Model::chainsChanged, videoNode, &VideoNode::setChains);

    // See if this VideoNode requests any chains
    auto requestedChains = videoNode->requestedChains();
    for (auto c = requestedChains.begin(); c != requestedChains.end(); c++) {
        if (!d_ptr->m_chains.contains(*c)) {
            addChain(*c);
        }
    }
    // and be notified of changes
    connect(videoNode, &VideoNode::requestedChainAdded, this, &Model::addChain);
    connect(videoNode, &VideoNode::requestedChainRemoved, this, &Model::removeChain);
}

void Model::disownNode(VideoNode *videoNode) {
    if(!videoNode)
        return;

    disconnect(this, &Model::chainsChanged, videoNode, &VideoNode::setChains);
    disconnect(videoNode, &VideoNode::requestedChainAdded, this, &Model::addChain);
    disconnect(videoNode, &VideoNode::requestedChainRemoved, this, &Model::removeChain);

    disconnect(videoNode, &VideoNode::inputCountChanged, this, &Model::flush);
    disconnect(videoNode, &VideoNode::message, this, &Model::onMessage);
    disconnect(videoNode, &VideoNode::warning, this, &Model::onWarning);
    disconnect(videoNode, &VideoNode::fatal, this, &Model::onFatal);

    auto requestedChains = videoNode->requestedChains();
    auto chains = d_ptr->m_chains;
    for (auto c = requestedChains.begin(); c != requestedChains.end(); c++) {
        d_ptr->m_chains.removeAll(*c);
    }

    if (chains != d_ptr->m_chains) {
        emit chainsChanged(d_ptr->m_chains);
    }
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

void Model::addVideoNode(VideoNode *videoNode) {
    if(!videoNode)
        return;
    if (!d_ptr->m_vertices.contains(videoNode)) {
        prepareNode(videoNode);
        d_ptr->m_vertices.append(videoNode);
    }
}

void Model::removeVideoNode(VideoNode *videoNode) {
    QList<Edge> removed;

    QMutableListIterator<Edge> i(d_ptr->m_edges);
    while (i.hasNext()) {
        auto edgeCopy = i.next();
        if (edgeCopy.fromVertex == videoNode || edgeCopy.toVertex == videoNode) {
            i.remove();
        }
    }

    disownNode(videoNode);
    d_ptr->m_vertices.removeAll(videoNode);

    if (videoNode->parent() == this) {
        videoNode->deleteLater();
    }
}

void Model::addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    if (fromVertex == nullptr
     || toVertex == nullptr
     || toInput < 0
     || !d_ptr->m_vertices.contains(fromVertex)
     || !d_ptr->m_vertices.contains(toVertex)) {
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
    QMutableListIterator<Edge> i(d_ptr->m_edges);
    while (i.hasNext()) {
        Edge edgeCopy = i.next();
        if (edgeCopy.toVertex == toVertex && edgeCopy.toInput == toInput) {
            if (edgeCopy.fromVertex == fromVertex)
                return;
            qInfo() << "Erasing" << edgeCopy.fromVertex << edgeCopy.toVertex << edgeCopy.toInput;
            i.remove();
        }
    }

    d_ptr->m_edges.append(newEdge);
    QVector<VideoNode *> sortedVertices = topoSort();
    if(sortedVertices.count() < d_ptr->m_vertices.count()) {
        // Roll back changes if a cycle was detected
        d_ptr->m_edges = edgesOld;
        qWarning() << QString("Not adding edge because it would create a cycle: %1 to %2 input %3").arg(vnp(fromVertex)).arg(vnp(toVertex)).arg(toInput);
        return;
    }
}

void Model::removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
    if (fromVertex == nullptr
     || toVertex == nullptr
     || toInput < 0
     || !d_ptr->m_vertices.contains(fromVertex)
     || !d_ptr->m_vertices.contains(toVertex)) {
        qWarning() << QString("Bad edge: %1 to %2 input %3").arg(vnp(fromVertex)).arg(vnp(toVertex)).arg(toInput);
        return;
    }

    Edge edgeCopy;
    QMutableListIterator<Edge> i(d_ptr->m_edges);
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
    QMutexLocker locker(&d_ptr->m_graphLock);
    ModelCopyForRendering out;

    // Create a map from VideoNodes to indices
    QMap<VideoNode *, int> map;
    for (int i=0; i<d_ptr->m_verticesSortedForRendering.count(); i++) {
        map.insert(d_ptr->m_verticesSortedForRendering.at(i), i);
    }

    for (int i=0; i<d_ptr->m_edgesForRendering.count(); i++) {
        out.fromVertex.append(map.value(d_ptr->m_edgesForRendering.at(i).fromVertex, -1));
        out.toVertex.append(map.value(d_ptr->m_edgesForRendering.at(i).toVertex, -1));
        out.toInput.append(d_ptr->m_edgesForRendering.at(i).toInput);
    }

    for (int i=0; i<d_ptr->m_verticesSortedForRendering.count(); i++) {
        out.vertices.append(QSharedPointer<VideoNode>(d_ptr->m_verticesSortedForRendering.at(i)->clone()));
    }

    return out;
}

QVector<VideoNode *> Model::topoSort() {
    // Kahn's algorithm from Wikipedia

    auto edges = d_ptr->m_edges;

    QVector<VideoNode *> l;
    QList<VideoNode *> s;
    {
        QSet<VideoNode *> sSet;
        // Populate s, the start nodes
        for (auto v = d_ptr->m_vertices.begin(); v != d_ptr->m_vertices.end(); v++) {
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
    return d_ptr->m_vertices;
}

QList<Edge> Model::edges() const {
    return d_ptr->m_edges;
}

QVariantList Model::qmlVertices() const {
    QVariantList vertices;

    for (auto vertex = d_ptr->m_vertices.begin(); vertex != d_ptr->m_vertices.end(); vertex++) {
        vertices.append(QVariant::fromValue(*vertex));
    }
    return vertices;
}

QVariantList Model::qmlEdges() const {
    QVariantList edges;

    for (auto edge = d_ptr->m_edges.begin(); edge != d_ptr->m_edges.end(); edge++) {
        QVariantMap vedge;
        vedge.insert("fromVertex", QVariant::fromValue(edge->fromVertex));
        vedge.insert("toVertex", QVariant::fromValue(edge->toVertex));
        vedge.insert("toInput", edge->toInput);
        edges.append(vedge);
    }
    return edges;
}

void Model::clear() {
    while (!d_ptr->m_vertices.empty()) {
        removeVideoNode(d_ptr->m_vertices[0]);
    }
}

void Model::flush() {
    QList<VideoNode *> verticesAdded;
    QList<VideoNode *> verticesRemoved;
    QList<Edge> edgesAdded;
    QList<Edge> edgesRemoved;

    // Prune invalid edges
    QMutableListIterator<Edge> i(d_ptr->m_edges);
    while (i.hasNext()) {
        auto edgeCopy = i.next();
        if (edgeCopy.toInput >= edgeCopy.toVertex->inputCount()) {
            qDebug() << QString("Removing invalid edge to %1 input %2").arg(vnp(edgeCopy.toVertex)).arg(edgeCopy.toInput);
            i.remove();
        }
    }

    // Compute the changeset
    {
        auto v = QSet<VideoNode *>::fromList(d_ptr->m_vertices);
        auto v4r = QSet<VideoNode *>::fromList(d_ptr->m_verticesForRendering);
        // TODO Can't use QSet without implementing qHash
        //auto e = QSet<Edge>::fromList(d_ptr->m_edges);
        //auto e4r = QSet<Edge>::fromList(d_ptr->m_edgesForRendering);
        auto e = d_ptr->m_edges;
        auto e4r = d_ptr->m_edgesForRendering;

        for (int i=0; i<d_ptr->m_vertices.count(); i++) {
            if (!v4r.contains(d_ptr->m_vertices.at(i))) verticesAdded.append(d_ptr->m_vertices.at(i));
        }
        for (int i=0; i<d_ptr->m_verticesForRendering.count(); i++) {
            if (!v.contains(d_ptr->m_verticesForRendering.at(i))) verticesRemoved.append(d_ptr->m_verticesForRendering.at(i));
        }
        for (int i=0; i<d_ptr->m_edges.count(); i++) {
            if (!e4r.contains(d_ptr->m_edges.at(i))) edgesAdded.append(d_ptr->m_edges.at(i));
        }
        for (int i=0; i<d_ptr->m_edgesForRendering.count(); i++) {
            if (!e.contains(d_ptr->m_edgesForRendering.at(i))) edgesRemoved.append(d_ptr->m_edgesForRendering.at(i));
        }
    }

    // Swap
    {
        QMutexLocker locker(&d_ptr->m_graphLock);
        d_ptr->m_verticesForRendering = d_ptr->m_vertices;
        d_ptr->m_edgesForRendering = d_ptr->m_edges;
        d_ptr->m_verticesSortedForRendering = topoSort();
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
        for (auto e : d_ptr->m_edges) {
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

QMap<VideoNode, GLuint> ModelCopyForRendering::render(Chain chain) {
    // inputs is parallel to vertices
    // and contains the VideoNodes connected to the
    // corresponding vertex's inputs
    QVector<QVector<int>> inputs;
    auto vao = chain.vao();

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
        QVector<GLuint> inputTextures(vertex->inputCount(), chain.blankTexture());
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

    QMap<VideoNode, GLuint> result;
    for (int i=0; i<resultTextures.count(); i++) {
        if (resultTextures.at(i) != 0) {
            result.insert(*vertices.at(i), resultTextures.at(i));
        }
    }
    vao->release();
    return result;
}

QJsonObject Model::serialize() {
    QJsonObject jsonOutput;

    // Create a map from VideoNodes to indices
    QMap<VideoNode *, int> map;
    QJsonArray jsonVertices;
    for (int i=0; i<d_ptr->m_vertices.count(); i++) {
        map.insert(d_ptr->m_vertices.at(i), i);
        jsonVertices.append(d_ptr->m_vertices.at(i)->serialize());
    }

    jsonOutput["vertices"] = jsonVertices;

    QJsonArray jsonEdges;
    for (auto edge : d_ptr->m_edges) {
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

    QList<VideoNode *> addedVertices;
    QJsonArray jsonVertices = data["vertices"].toArray();
    for (auto _jsonVertex : jsonVertices) {
        QJsonObject jsonVertex = _jsonVertex.toObject();
        VideoNode *vertex = registry->deserialize(context, jsonVertex);
        if (vertex != nullptr) {
            addVideoNode(vertex);
        }
        addedVertices.append(vertex);
    }

    QJsonArray jsonEdges = data["edges"].toArray();
    for (auto _jsonEdge : jsonEdges) {
        QJsonObject jsonEdge = _jsonEdge.toObject();
        VideoNode *fromVertex = addedVertices.at(jsonEdge["fromVertex"].toInt());
        VideoNode *toVertex = addedVertices.at(jsonEdge["toVertex"].toInt());
        int toInput = jsonEdge["toInput"].toInt();
        addEdge(fromVertex, toVertex, toInput);
    }
}

void Model::load(Context *context, Registry *registry, QString name) {
    QString filename(QDir(Paths::models()).filePath(QString("%1.json").arg(name)));
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

void Model::save(QString name) {
    QString filename(QDir(Paths::models()).filePath(QString("%1.json").arg(name)));
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open file for writing:" << filename;
        return;
    }
    QJsonDocument doc(serialize());
    file.write(doc.toJson());
}

WeakModel::WeakModel()
{
}

WeakModel::WeakModel(const Model &other)
    : d_ptr(other.d_ptr)
{
}

ModelCopyForRendering WeakModel::createCopyForRendering() {
    auto strong_ptr = d_ptr.toStrongRef();
    if (!strong_ptr) return ModelCopyForRendering();
    return Model(strong_ptr).createCopyForRendering();
}

ModelPrivate::ModelPrivate()
{
}
