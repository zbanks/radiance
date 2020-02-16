#include "View.h"
#include "VideoNode.h"
#include "Paths.h"
#include <QQmlContext>
#include <QQmlProperty>
#include <QFileInfo>
#include <QQueue>
#include <QtGlobal>

View::View()
    : m_model(nullptr)
{
    setFlag(ItemHasContents, true);
}

void View::componentComplete() {
    QQuickItem::componentComplete();
    auto ao = qobject_cast<ControlsAttachedType *>(qmlAttachedPropertiesObject<Controls>(this));
    connect(ao, &ControlsAttachedType::controlChangedAbs, this, &View::onControlChangedAbs);
    connect(ao, &ControlsAttachedType::controlChangedRel, this, &View::onControlChangedRel);
}

View::~View() {
}

void View::rebuild() {
    for (auto c = m_children.begin(); c != m_children.end(); c++) {
        c->item->deleteLater();
    }
    m_children.clear();
    m_selection.clear();
    onGraphChanged();
}

Child View::newChild(VideoNodeSP *videoNode) {
    Child c;
    c.videoNode = videoNode;
    c.item = nullptr;

    auto delegate = m_delegates.value((*videoNode)->metaObject()->className());
    if (delegate.isEmpty()) {
        delegate = m_delegates.value("");
        if (delegate.isEmpty()) {
            qFatal("Could not find a delegate for %s", videoNode->metaObject()->className());
        }
    }
    auto qmlFileInfo = QFileInfo(Paths::qml() + QString("/%1.qml").arg(delegate));
    QQmlEngine *engine = QQmlEngine::contextForObject(this)->engine();
    QQmlComponent component(engine, QUrl::fromLocalFile(qmlFileInfo.absoluteFilePath()));
    if( component.status() != QQmlComponent::Ready )
    {
        if(component.status() == QQmlComponent::Error) {
            qDebug() << component.errorString();
            qFatal("Could not construct delegate");
        }
    }
    auto item = qobject_cast<BaseVideoNodeTile *>(component.create());

    item->setParentItem(this);
    item->setProperty("videoNode", QVariant::fromValue(videoNode));
    item->setProperty("model", QVariant::fromValue(new ModelSP(m_model)));
    c.item = item;

    return c;
}

static void setInputHeightFwd(const QVector<QVector<int>> &inputs, const QVector<QVector<qreal>> &minInputHeights, QVector<QVector<int>> &inputGridHeight, QVector<QVector<qreal>> &inputHeight, int node) {
    auto myInputs = inputs.at(node);
    for(int i=0; i<myInputs.count(); i++) {
        int attachedNode = myInputs.at(i);
        inputHeight[node][i] = minInputHeights.at(node).at(i);
        inputGridHeight[node][i] = 1;
        if (attachedNode >= 0) {
            setInputHeightFwd(inputs, minInputHeights, inputGridHeight, inputHeight, attachedNode);
            auto childInputGridHeights = inputGridHeight.at(attachedNode);
            auto childInputHeights = inputHeight.at(attachedNode);
            Q_ASSERT(childInputGridHeights.count() == childInputHeights.count());
            int myInputGridHeight = 0;
            qreal myInputHeight = 0;
            for(int j=0; j<childInputGridHeights.count(); j++) {
                Q_ASSERT(childInputGridHeights.at(j) > 0);
                Q_ASSERT(childInputHeights.at(j) > 0);
                myInputGridHeight += childInputGridHeights.at(j);
                myInputHeight += childInputHeights.at(j);
            }
            if (myInputGridHeight > inputGridHeight.at(node).at(i)) {
                inputGridHeight[node][i] = myInputGridHeight;
            }
            if (myInputHeight > inputHeight.at(node).at(i)) {
                inputHeight[node][i] = myInputHeight;
            }
        }
    }
}

static void setInputHeightRev(const QVector<QVector<int>> &inputs, QVector<QVector<qreal>> &inputHeight, int node) {
    auto myInputs = inputs.at(node);
    for (int i=0; i<myInputs.count(); i++) {
        auto attachedNode = myInputs.at(i);
        auto myInputHeight = inputHeight[node][i];
        if (attachedNode >= 0) {
            auto childInputHeights = inputHeight.at(attachedNode);
            qreal childHeight = 0;
            for(int j=0; j<childInputHeights.count(); j++) childHeight += childInputHeights.at(j);
            if (childHeight < myInputHeight) {
                auto correctionFactor = myInputHeight / childHeight;
                for(int j=0; j<childInputHeights.count(); j++) inputHeight[attachedNode][j] *= correctionFactor;
            }
            setInputHeightRev(inputs, inputHeight, attachedNode);
        }
    }
}

static void setLayer(const QVector<QVector<int>> &inputs, const QVector<qreal> &widths, QVector<int> &layers, QVector<qreal> &xs, int node, int layer, qreal x) {
    layers[node] = layer;
    xs[node] = x + widths.at(node);
    auto myInputs = inputs.at(node);
    for(int i=0; i<myInputs.count(); i++) {
        int attachedNode = myInputs.at(i);
        if (attachedNode >= 0) {
            setLayer(inputs, widths, layers, xs, attachedNode, layer + 1, x + widths.at(node));
        }
    }
}

static void setStackup(const QVector<QVector<int>> &inputs, const QVector<QVector<int>> &inputGridHeight, const QVector<QVector<qreal>> &inputHeight, QVector<int> &stacks, QVector<qreal> &ys, int node, int stack, qreal y) {
    stacks[node] = stack;
    ys[node] = y;
    auto myInputs = inputs.at(node);
    auto myInputGridHeights = inputGridHeight.at(node);
    auto myInputHeights = inputHeight.at(node);
    for(int i=0; i<myInputs.count(); i++) {
        int attachedNode = myInputs.at(i);
        if (attachedNode >= 0) {
            setStackup(inputs, inputGridHeight, inputHeight, stacks, ys, attachedNode, stack, y);
        }
        stack += myInputGridHeights.at(i);
        y += myInputHeights.at(i);
    }
}

QQuickItem *View::createDropArea() {
    auto qmlFileInfo = QFileInfo(Paths::qml() + "/TileDropArea.qml");
    QQmlEngine *engine = QQmlEngine::contextForObject(this)->engine();
    QQmlComponent component(engine, QUrl::fromLocalFile(qmlFileInfo.absoluteFilePath()));
    if( component.status() != QQmlComponent::Ready )
    {
        if(component.status() == QQmlComponent::Error) {
            qDebug() << component.errorString();
            qFatal("Could not construct TileDropArea");
        }
    }
    auto item = qobject_cast<QQuickItem *>(component.create());

    item->setParentItem(this);
    item->setParent(this);
    return item;
}

void View::onGraphChanged() {
    if (m_model == nullptr) return;

    QMap<VideoNodeSP *, int> existingChildMap;
    for (int i=0; i<m_children.count(); i++) {
        existingChildMap.insert(m_children.at(i).videoNode, i);
    }

    QList<Child> newChildren;
    auto vertices = (*m_model)->vertices();
    auto edges = (*m_model)->edges();

    for (auto v = vertices.begin(); v != vertices.end(); v++) {
        auto oldChild = existingChildMap.value(*v, -1);
        if (oldChild != -1) {
            newChildren.append(m_children.at(oldChild));
        } else {
            newChildren.append(newChild(*v));
        }
    }

    // Create a map from VideoNodes to indices
    QMap<VideoNodeSP *, int> map;
    for (int i=0; i<vertices.count(); i++) {
        map.insert(vertices.at(i), i);
    }

    // Delete children that were removed
    for (auto c = m_children.begin(); c != m_children.end(); c++) {
        if (!map.contains(c->videoNode)) {
            c->item->deleteLater();
        }
    }

    m_children = newChildren;

    // Create a list of heights parallel to vertices
    QVector<QVector<qreal>> minHeights(m_children.count());
    for (int i=0; i<m_children.count(); i++) {
        auto index = map.value(m_children.at(i).videoNode, -1);
        Q_ASSERT(index >= 0);
        QVariant heightVar = m_children.at(i).item->property("minInputHeight");
        auto inputCount = (*m_children.at(i).videoNode)->inputCount();
        if (inputCount < 1) inputCount = 1; // Handle zero-input nodes
        if (heightVar.canConvert(QMetaType::QVariantList)) {
            QVariantList heightList = heightVar.toList();
            for (int j=0; j<inputCount; j++) {
                if (j < heightList.count()) {
                    minHeights[index].append(heightList.at(j).toReal());
                } else if (!heightList.empty()) {
                    minHeights[index].append(heightList.last().toReal());
                } else {
                    minHeights[index].append(0);
                }
            }
        } else {
            qreal height = heightVar.toReal();
            for (int j=0; j<inputCount; j++) {
                minHeights[index].append(height);
            }
        }
    }

    // Fill these two lists with indices
    // They are parallel to edges
    QVector<int> fromVertex;
    QVector<int> toVertex;
    for(int i=0; i<edges.count(); i++) {
        fromVertex.append(map.value(edges.at(i).fromVertex, -1));
        toVertex.append(map.value(edges.at(i).toVertex, -1));
    }

    QVector<QVector<int>> inputs;
    QVector<QVector<int>> inputGridHeight;
    QVector<QVector<qreal>> inputHeight;
    QVector<int> gridX(vertices.count(), -1);
    QVector<qreal> xs(vertices.count(), -1);
    QVector<int> gridY(vertices.count(), -1);
    QVector<qreal> ys(vertices.count(), -1);

    // Create a list of -1's
    for (int i=0; i<vertices.count(); i++) {
        auto inputCount = (*vertices.at(i))->inputCount();
        if (inputCount < 1) inputCount = 1;
        inputs.append(QVector<int>(inputCount, -1));
        inputGridHeight.append(QVector<int>(inputCount, -1));
        inputHeight.append(QVector<qreal>(inputCount, -1));
    }

    // Create a vector for looking up a node's inputs
    for (int i = 0; i < toVertex.count(); i++) {
        auto to = toVertex.at(i);
        if (to >= 0 && edges.at(i).toInput < inputs.at(to).count()) {
            inputs[to][edges.at(i).toInput] = fromVertex.at(i);
        }
    }

    // Find the root nodes
    QVector<int> s;
    QSet<VideoNodeSP *> sSet;
    // Populate s, the start nodes
    for (int i=0; i<vertices.count(); i++) {
        sSet.insert(vertices.at(i));
    }
    for (auto e = edges.begin(); e != edges.end(); e++) {
        if (e->toInput < (*e->toVertex)->inputCount()) sSet.remove(e->fromVertex);
    }
    for (int i=0; i<vertices.count(); i++) {
        if (sSet.contains(vertices.at(i))) {
            s.append(i);
        }
    }

    // Then we deal with heights and Y,
    // in case widths are dependent on heights

    // Compute heights and Y positions
    int stack = 0;
    qreal totalHeight = 0;
    for (int i=0; i<s.count(); i++) {
        setInputHeightFwd(inputs, minHeights, inputGridHeight, inputHeight, s.at(i));
        setInputHeightRev(inputs, inputHeight, s.at(i));
        setStackup(inputs, inputGridHeight, inputHeight, gridY, ys, s.at(i), stack, totalHeight);
        auto myInputGridHeights = inputGridHeight.at(s.at(i));
        auto myInputHeights = inputHeight.at(s.at(i));
        Q_ASSERT(myInputGridHeights.count() == myInputHeights.count());
        for (int j=0; j<myInputGridHeights.count(); j++) {
            stack += myInputGridHeights.at(j);
            totalHeight += myInputHeights.at(j);
        }
    }

    // Assign heights and Ys
    for (int i=0; i<m_children.count(); i++) {
        QVariantList gridHeightsVar;
        auto myInputGridHeights = inputGridHeight.at(i);
        for (int j=0; j<myInputGridHeights.count(); j++) {
            gridHeightsVar.append(myInputGridHeights.at(j));
        }
        QVariantList heightsVar;
        auto myInputHeights = inputHeight.at(i);
        for (int j=0; j<myInputHeights.count(); j++) {
            heightsVar.append(myInputHeights.at(j));
        }
        m_children[i].item->setProperty("inputHeights", heightsVar);
        m_children[i].item->setProperty("posY", ys.at(i));
        m_children[i].item->setProperty("inputGridHeights", gridHeightsVar);
        m_children[i].item->setProperty("gridY", gridY.at(i));
    }

    // Finally we deal with widths and X,
    // in case they are dependent on heights and Y

    // Create a list of widths parallel to vertices
    QVector<qreal> widths(m_children.count(), -1);
    for (int i=0; i<m_children.count(); i++) {
        auto index = map.value(m_children.at(i).videoNode, -1);
        Q_ASSERT(index >= 0);
        widths[index] = m_children.at(i).item->property("blockWidth").toReal();
    }

    // Compute widths and X positions
    for (int i=0; i<s.count(); i++) {
        setLayer(inputs, widths, gridX, xs, s.at(i), 0, 0);
    }

    // Find the bounds of the whole graph
    qreal totalWidth = 0;
    for (int i=0; i<xs.count(); i++) {
        if (xs.at(i) > totalWidth) totalWidth = xs.at(i);
    }

    // Reverse X ordering
    for (int i=0; i<xs.count(); i++) {
        xs[i] = totalWidth - xs.at(i);
    }

    // Assign width and X
    for (int i=0; i<m_children.count(); i++) {
        m_children[i].item->setProperty("posX", xs.at(i));
        m_children[i].item->setProperty("gridX", gridX.at(i));
    }

    // Now let's do some tab ordering.
    // We tab order the nodes using a reverse-BFS
    // because @zbanks thinks it's cool

    QVector<int> sortedNodes;
    QQueue<int> bfsQueue;
    for (int i=s.count() - 1; i>=0; i--) {
        bfsQueue.append(s.at(i));
        while (!bfsQueue.isEmpty()) {
            auto n = bfsQueue.dequeue();
            sortedNodes.append(n);
            auto toAdd = inputs.at(n);
            for (int j=toAdd.count()-1; j>=0; j--) {
                if (toAdd.at(j) >= 0) bfsQueue.enqueue(toAdd.at(j));
            }
        }
    }

    for (int i=0; i<sortedNodes.count(); i++) {
        auto cur = sortedNodes.at(i);
        auto next = sortedNodes.at((i + 1) % sortedNodes.count());
        auto prev = sortedNodes.at((i + sortedNodes.count() - 1) % sortedNodes.count());

        m_children[cur].item->setProperty("tab", QVariant::fromValue(m_children[prev].item));
        m_children[cur].item->setProperty("backtab", QVariant::fromValue(m_children[next].item));
    }

    QList<QQuickItem *> dropAreas;
    for (int i=0; i<m_children.count(); i++) {
        auto myInputGridHeights = inputGridHeight.at(i);
        auto myInputHeights = inputHeight.at(i);
        auto vertex = m_children.at(i).videoNode;
        Q_ASSERT(myInputGridHeights.count() == myInputHeights.count());
        qreal sumInputHeights = 0;

        for (int j=0; j<(*vertex)->inputCount(); j++) {
            // Create a drop area at each input of every node
            auto item = createDropArea();

            item->setProperty("posX", xs.at(i));
            item->setProperty("posY", ys.at(i) + sumInputHeights);
            sumInputHeights += myInputHeights.at(j);
            item->setProperty("posHeight", myInputHeights.at(j));
            item->setProperty("gridX", gridX.at(i) + 0.5);
            item->setProperty("gridY", gridY.at(i) + j);
            item->setProperty("gridHeight", myInputGridHeights.at(j));
            QVariant fromNode = QVariant::fromValue(static_cast<VideoNodeSP *>(nullptr));

            // TODO this makes this N^2, a clever QMap could solve this
            for (int k=0; k<edges.count(); k++) {
                if (edges.at(k).toVertex == vertex
                 && edges.at(k).toInput == j) {
                    fromNode.setValue(edges.at(k).fromVertex);
                    break;
                }
            }

            item->setProperty("fromNode", fromNode);
            item->setProperty("toNode", QVariant::fromValue(vertex));
            item->setProperty("toInput", j);
            dropAreas.append(item);
        }
        // Create a drop area at the output of root nodes
        if (sSet.contains(vertex)) {
            auto item = createDropArea();
            int totalGridHeight = 0;
            int totalHeight = 0;
            for (int j=0; j<myInputGridHeights.count(); j++) {
                totalGridHeight += myInputGridHeights.at(j);
                totalHeight += myInputHeights.at(j);
            }
            item->setProperty("posX", xs.at(i) + widths.at(i));
            item->setProperty("posY", ys.at(i));
            item->setProperty("posHeight", totalHeight);
            item->setProperty("gridX", gridX.at(i) - 0.5);
            item->setProperty("gridY", gridY.at(i));
            item->setProperty("gridHeight", totalGridHeight);
            item->setProperty("fromNode", QVariant::fromValue(vertex));
            item->setProperty("toNode", QVariant::fromValue(static_cast<VideoNodeSP *>(nullptr)));
            item->setProperty("toInput", -1);
            dropAreas.append(item);
        }
    }
    // Create a drop area for starting a new row
    {
        auto item = createDropArea();
        item->setProperty("posX", totalWidth);
        item->setProperty("posY", totalHeight);
        item->setProperty("gridX", -0.5);
        item->setProperty("gridY", stack);
        item->setProperty("gridHeight", 1);
        item->setProperty("fromNode", QVariant::fromValue(static_cast<VideoNodeSP *>(nullptr)));
        item->setProperty("toNode", QVariant::fromValue(static_cast<VideoNodeSP *>(nullptr)));
        item->setProperty("toInput", -1);
        totalHeight += item->property("posHeight").toReal();
        dropAreas.append(item);
    }

    setWidth(totalWidth);
    setHeight(totalHeight);

    qDeleteAll(m_dropAreas.begin(), m_dropAreas.end());
    m_dropAreas = dropAreas;

    addToSelection(selection());
    selectionChanged();
}

void View::selectionChanged() {
    QSet<BaseVideoNodeTile *> found;
    for (int i=0; i<m_children.count(); i++) {
        auto selected = m_selection.contains(m_children.at(i).item);
        m_children[i].item->setProperty("selected", selected);
        if (selected) found.insert(m_children.at(i).item);
    }
    m_selection = found;
}

ModelSP *View::model() {
    return m_model;
}

void View::setModel(ModelSP *model) {
    if(m_model != nullptr) disconnect(model, nullptr, this, nullptr);
    m_model = model;
    if(m_model != nullptr) {
        connect(m_model->data(), &Model::graphChanged, this, &View::onGraphChanged);
    }
    rebuild();
    emit modelChanged(m_model);
}

QMap<QString, QString> View::delegates() {
    return m_delegates;
}

QVariantMap View::qml_delegates() {
    QVariantMap map;
    for (auto k = m_delegates.keyBegin(); k != m_delegates.keyEnd(); k++) {
        auto v = m_delegates.value(*k);
        map.insert(*k, v);
    }
    return map;
}

void View::setDelegates(QMap<QString, QString> value) {
    rebuild();
    emit delegatesChanged(m_delegates);
    emit qml_delegatesChanged(qml_delegates());
}

void View::qml_setDelegates(QVariantMap value) {
    m_delegates.clear();
    for (auto k = value.keyBegin(); k != value.keyEnd(); k++) {
        auto v = value.value(*k);
        m_delegates.insert(*k, v.toString());
    }
    rebuild();
    emit delegatesChanged(m_delegates);
    emit qml_delegatesChanged(qml_delegates());
}

void View::select(QVariantList _tiles) {
    QVariantList tiles = frozenConnectedComponents(_tiles);
    m_selection.clear();
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<BaseVideoNodeTile*>(tiles[i]);
        m_selection.insert(tile);
    }
    selectionChanged();
}

void View::addToSelection(QVariantList _tiles) {
    QVariantList tiles = frozenConnectedComponents(_tiles);
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<BaseVideoNodeTile*>(tiles[i]);
        m_selection.insert(tile);
    }
    selectionChanged();
}

void View::removeFromSelection(QVariantList _tiles) {
    QVariantList tiles = frozenConnectedComponents(_tiles);
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<BaseVideoNodeTile*>(tiles[i]);
        m_selection.remove(tile);
    }
    selectionChanged();
}

void View::toggleSelection(QVariantList _tiles) {
    QVariantList tiles = frozenConnectedComponents(_tiles);
    bool allSelected = true;
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<BaseVideoNodeTile*>(tiles[i]);
        if (!m_selection.contains(tile)) {
            allSelected = false;
        }
    }
    if (allSelected) {
        removeFromSelection(tiles);
    } else {
        addToSelection(tiles);
    }
}

void View::ensureSelected(QVariantList _tiles) {
    QVariantList tiles = frozenConnectedComponents(_tiles);
    bool allSelected = true;
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<BaseVideoNodeTile*>(tiles[i]);
        if (!m_selection.contains(tile)) {
            allSelected = false;
        }
    }
    if (!allSelected) {
        m_selection.clear();
        addToSelection(tiles);
    }
}

QVariantList View::selection() {
    QVariantList selection;
    for (int i=0; i<m_children.count(); i++) {
        if (m_selection.contains(m_children.at(i).item)) {
            selection.append(QVariant::fromValue(m_children.at(i).item));
        }
    }
    return selection;
}

BaseVideoNodeTile *View::focusedChild() {
    for (int i=0; i<m_children.count(); i++) {
        if (m_children.at(i).item->hasActiveFocus()) {
            return m_children.at(i).item;
        }
    }
    return nullptr;
}

static void setColor(const QVector<QVector<int>> &edges, QVector<int> &colors, int node, int &curColor) {
    if (colors.at(node) != 0) return;
    int myColor = 0;
    bool newColor = false;
    for (int i=0; i<edges.at(node).count(); i++) {
        auto child = edges.at(node).at(i);
        setColor(edges, colors, child, curColor);
        Q_ASSERT(colors.at(child) != 0);
        if (myColor != 0 && colors.at(child) != myColor) newColor = true;
        myColor = colors.at(child);
    }
    if (newColor || myColor == 0) myColor = curColor++;
    colors[node] = myColor;
}

static void findColoredInputs(const QVector<QVector<int>> &revEdges, const QVector<int> &colors, int node, int color, QVector<int> &inputNodes, QVector<int> &inputPorts) {
    for (int i=0; i<revEdges.at(node).count(); i++) {
        auto parent = revEdges.at(node).at(i);
        if (parent < 0 || colors.at(parent) != color) {
            inputNodes.append(node);
            inputPorts.append(i);
        } else {
            findColoredInputs(revEdges, colors, parent, color, inputNodes, inputPorts);
        }
    }
}

QVariantList View::frozenConnectedComponents(QVariantList tiles) {
    QList<VideoNodeSP *> queuedTiles;
    QSet<VideoNodeSP *> connectedTiles;

    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<BaseVideoNodeTile*>(tiles[i]);
        Q_ASSERT(tile != nullptr);
        VideoNodeSP *vertex = tile->videoNode();
        queuedTiles.append(vertex);
        connectedTiles.insert(vertex);
    }

    auto edges = (*m_model)->edges();
    while (!queuedTiles.isEmpty()) {
        VideoNodeSP *vertex = queuedTiles.takeLast();
        if (!(*vertex)->frozenInput() && !(*vertex)->frozenOutput()) {
            continue;
        }
        for (auto edge : edges) {
            VideoNodeSP *adjVertex = nullptr;
            if ((*vertex)->frozenInput() && edge.toVertex == (vertex)) {
                adjVertex = edge.fromVertex;
            } else if ((*vertex)->frozenOutput() && edge.fromVertex == (vertex)) {
                adjVertex = edge.toVertex;
            } else {
                continue;
            }
            Q_ASSERT(adjVertex != nullptr);
            if (!connectedTiles.contains(adjVertex)) {
                connectedTiles.insert(adjVertex);
                queuedTiles.append(adjVertex);
            }
        }
    }

    QVariantList output;
    for (auto node : connectedTiles) { // TODO: This is secretly O(n^2) because of tileForVideoNode
        output.append(QVariant::fromValue(tileForVideoNode(node)));
    }
    return output;
}

QVariantList View::selectedConnectedComponents() {
    // * tiles = A QVariantList of tiles contained within the connected component
    // * vertices = A QVariantList of vertices (VideoNodes) contained within the connected component
    // * edges = A QVariantList of edges contained within the connected component
    // * inputEdges = A QVariantList of input edges to the connected component (ordered)
    // * outputEdges = A QVariantList of output edges from the connected component (unordered)
    // * outputNode = The output VideoNode

    // First, we color each node of the subgraph according to the following algorithm:
    //    If all of the children have the same color, use that color
    //    If the children have different colors, or there are no children, pick a new unique color.

    // We then return the subgraph consisting of nodes that have the same color as the
    // given node.

    // Find only the vertices contained in the selection
    auto vertices = (*m_model)->vertices();
    QSet<VideoNodeSP *> selectedVerticesSet;
    QVector<VideoNodeSP *> selectedVertices;
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        auto vSet = QSet<VideoNodeSP *>::fromList(vertices);
#else
        auto vSet = QSet<VideoNodeSP *>(vertices.begin(), vertices.end());
#endif
        for (int i=0; i<m_children.count(); i++) {
            if (m_selection.contains(m_children.at(i).item)
             && vSet.contains(m_children.at(i).videoNode)) {
                selectedVertices.append(m_children.at(i).videoNode);
                selectedVerticesSet.insert(m_children.at(i).videoNode);
            }
        }
    }

    // Find only the edges contained in the selection
    QVector<Edge> selectedEdges;
    auto edges = (*m_model)->edges();
    for (int i=0; i<edges.count(); i++) {
        if (selectedVerticesSet.contains(edges.at(i).fromVertex)
         && selectedVerticesSet.contains(edges.at(i).toVertex)) {
            selectedEdges.append(edges.at(i));
        }
    }

    // Create a map from VideoNodes to indices
    QMap<VideoNodeSP *, int> map;
    for (int i=0; i<selectedVertices.count(); i++) {
        map.insert(selectedVertices.at(i), i);
    }

    // Create a data structure for looking up edges
    QVector<QVector<int>> edgeLookup(selectedVertices.count());
    for (int i=0; i<selectedEdges.count(); i++) {
        auto fromIndex = map.value(selectedEdges.at(i).fromVertex, -1);
        auto toIndex = map.value(selectedEdges.at(i).toVertex, -1);
        Q_ASSERT(fromIndex >= 0);
        Q_ASSERT(toIndex >= 0);
        edgeLookup[fromIndex].append(toIndex);
    }

    QVector<int> colors(selectedVertices.count(), 0);
    int curColor = 1;
    // Color the graph
    for (int i=0; i<selectedVertices.count(); i++) {
        setColor(edgeLookup, colors, i, curColor);
    }

    // Create a data structure for looking up edges in reverse
    QVector<QVector<int>> edgeRevLookup(selectedVertices.count());
    for (int i=0; i<selectedVertices.count(); i++) {
        edgeRevLookup[i] = QVector<int>((*selectedVertices.at(i))->inputCount(), -1);
    }
    for (int i=0; i<selectedEdges.count(); i++) {
        auto fromIndex = map.value(selectedEdges.at(i).fromVertex, -1);
        auto toIndex = map.value(selectedEdges.at(i).toVertex, -1);
        auto toPort = selectedEdges.at(i).toInput;
        Q_ASSERT(fromIndex >= 0);
        Q_ASSERT(toIndex >= 0);
        edgeRevLookup[toIndex][toPort] = fromIndex;
    }

    // Loop over each color and populate the output
    QVariantList result;
    for (int c=1; c<curColor; c++) { // TODO this is a little bit N^2
        // Find all tiles of that color
        QVariantList tiles;
        QSet<VideoNodeSP *> coloredVerticesSet;
        for (int i=0; i<m_children.count(); i++) {
            auto tileIndex = map.value(m_children.at(i).videoNode, -1);
            if (tileIndex >= 0 && colors.at(tileIndex) == c) {
                tiles.append(QVariant::fromValue(m_children.at(i).item));
                coloredVerticesSet.insert(m_children.at(i).videoNode);
            }
        }
        // Find all vertices of that color
        QVariantList componentVertices;
        for (int i=0; i<vertices.count(); i++) {
            if (coloredVerticesSet.contains(vertices.at(i))) {
                componentVertices.append(QVariant::fromValue(vertices.at(i)));
            }
        }
        // Find all relevent edges
        QVariantList componentEdges;
        QVariantList inputEdges;
        QVariantList outputEdges;
        for (int i=0; i<edges.count(); i++) {
            auto containsFrom = coloredVerticesSet.contains(edges.at(i).fromVertex);
            auto containsTo = coloredVerticesSet.contains(edges.at(i).toVertex);
            if (containsFrom && containsTo) {
                componentEdges.append(edges.at(i).toVariantMap());
            } else if (containsFrom) {
                outputEdges.append(edges.at(i).toVariantMap());
            } else if (containsTo) {
                inputEdges.append(edges.at(i).toVariantMap());
            }
        }

        // Find the output node (the node with no outgoing edges)
        QSet<VideoNodeSP *> sSet = coloredVerticesSet;
        for (auto e = selectedEdges.begin(); e != selectedEdges.end(); e++) {
            auto toIndex = map.value(e->toVertex, -1);
            auto fromIndex = map.value(e->toVertex, -1);
            if (toIndex >= 0 && fromIndex >= 0
             && colors.at(toIndex) == c && colors.at(fromIndex) == c) {
                sSet.remove(e->fromVertex);
            }
        }

        Q_ASSERT(sSet.count() == 1);
        auto outputNode = *sSet.begin();

        // Find the input ports
        QVector<int> inputNodes;
        QVector<int> inputPorts;
        auto node = map.value(outputNode, -1);
        Q_ASSERT(node >= 0);
        findColoredInputs(edgeRevLookup, colors, node, c, inputNodes, inputPorts);
        QVariantList ports;
        for (int i=0; i<inputNodes.count(); i++) {
            QVariantMap port;
            port.insert("vertex", QVariant::fromValue(selectedVertices.at(inputNodes.at(i))));
            port.insert("input", inputPorts.at(i));
            ports.append(port);
        }

        QVariantMap obj;
        obj.insert("tiles", tiles);
        obj.insert("vertices", componentVertices);
        obj.insert("edges", componentEdges);
        obj.insert("inputEdges", inputEdges);
        obj.insert("outputEdges", outputEdges);
        obj.insert("inputPorts", ports);
        obj.insert("outputNode", QVariant::fromValue(outputNode));
        result.append(obj);
    }

    return result;
}

static void findAllPaths(int n, int end, const QVector<QVector<int>> &edges, QVector<int> path, QSet<int> &visited, QSet<int> &result) {
    if (visited.contains(n)) return;
    path.append(n);
    if (n == end) {
        for (int i=0; i<path.count(); i++) {
            result.insert(path.at(i));
        }
        return;
    }
    visited.insert(n);
    auto outgoingEdges = edges.at(n);
    for (int i=0; i<outgoingEdges.count(); i++) {
        findAllPaths(outgoingEdges.at(i), end, edges, path, visited, result);
    }
}

QVariantList View::tilesBetween(BaseVideoNodeTile *tile1, BaseVideoNodeTile *tile2) {
    // Create a map from VideoNodes to indices
    auto vertices = (*m_model)->vertices();
    QMap<VideoNodeSP *, int> map;
    for (int i=0; i<vertices.count(); i++) {
        map.insert(vertices.at(i), i);
    }

    // Find only the vertices contained in the selection
    int n1 = -1;
    int n2 = -1;
    for (int i=0; i<m_children.count(); i++) {
        if (tile1 == m_children.at(i).item) n1 = map.value(m_children.at(i).videoNode, -1);
        if (tile2 == m_children.at(i).item) n2 = map.value(m_children.at(i).videoNode, -1);
    }

    if (n1 < 0 || n2 < 0) {
        qWarning() << "Could not find tiles" << tile1 << tile2;
        return QVariantList();
    }

    // Create a data structure for looking up edges
    auto edges = (*m_model)->edges();
    QVector<QVector<int>> edgeLookup(vertices.count());
    for (int i=0; i<edges.count(); i++) {
        auto fromIndex = map.value(edges.at(i).fromVertex, -1);
        auto toIndex = map.value(edges.at(i).toVertex, -1);
        Q_ASSERT(fromIndex >= 0);
        Q_ASSERT(toIndex >= 0);
        edgeLookup[fromIndex].append(toIndex);
    }

    // Do DFS in both directions, finding all possible paths between the nodes
    QSet<int> result;
    QSet<int> visited;
    findAllPaths(n1, n2, edgeLookup, QVector<int>(), visited, result);
    visited.clear();
    findAllPaths(n2, n1, edgeLookup, QVector<int>(), visited, result);

    QVariantList tilesBetween;

    for (int i=0; i<m_children.count(); i++) {
        if (result.contains(map.value(m_children.at(i).videoNode, -1))) {
            tilesBetween.append(QVariant::fromValue(m_children.at(i).item));
        }
    }
    return tilesBetween;
}

BaseVideoNodeTile *View::tileForVideoNode(VideoNodeSP *videoNode) {
    if (videoNode == nullptr) return nullptr;
    for (auto child : m_children) {
        if (child.videoNode == videoNode)
            return child.item;
    }
    return nullptr;
}

void View::onControlChangedAbs(int bank, Controls::Control control, qreal value) {
    if (control >= Controls::Parameter0 && control <= Controls::Parameter9) {
        for (int i=0; i<m_children.count(); i++) {
            auto tile = m_children.at(i).item;
            if (tile->property("bank").toInt() == bank) {
                auto controls = qobject_cast<ControlsAttachedType *>(qmlAttachedPropertiesObject<Controls>(tile));
                controls->changeControlAbs(bank, control, value);
            }
        }
    } else {
        for (int i=0; i<m_children.count(); i++) {
            auto tile = m_children.at(i).item;
            if (m_selection.contains(tile)) {
                auto controls = qobject_cast<ControlsAttachedType *>(qmlAttachedPropertiesObject<Controls>(tile));
                controls->changeControlAbs(bank, control, value);
            }
        }
    }
}

void View::onControlChangedRel(int bank, Controls::Control control, qreal value) {
    if (control >= Controls::Parameter0 && control <= Controls::Parameter9) {
        for (int i=0; i<m_children.count(); i++) {
            auto tile = m_children.at(i).item;
            if (tile->property("bank").toInt() == bank) {
                auto controls = qobject_cast<ControlsAttachedType *>(qmlAttachedPropertiesObject<Controls>(tile));
                controls->changeControlRel(bank, control, value);
            }
        }
    } else if (control == Controls::Scroll) {
        auto tile = focusedChild();
        if (tile != nullptr) {
            auto controls = qobject_cast<ControlsAttachedType *>(qmlAttachedPropertiesObject<Controls>(tile));
            controls->changeControlRel(bank, control, value);
        }
    } else {
        for (int i=0; i<m_children.count(); i++) {
            auto tile = m_children.at(i).item;
            if (m_selection.contains(tile)) {
                auto controls = qobject_cast<ControlsAttachedType *>(qmlAttachedPropertiesObject<Controls>(tile));
                controls->changeControlRel(bank, control, value);
            }
        }
    }
}
