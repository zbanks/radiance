#include "View.h"
#include "main.h"
#include <QQmlContext>
#include <QQmlProperty>
#include <QFileInfo>

View::View()
    : m_model(nullptr)
{
    setFlag(ItemHasContents, true);
}

View::~View() {
}

void View::rebuild() {
    m_children.clear();
    m_selection.clear();
    onGraphChanged();
}

Child View::newChild(VideoNode *videoNode) {
    Child c;
    c.videoNode = videoNode;
    c.item = nullptr;

    auto delegate = m_delegates.value(videoNode->metaObject()->className());
    if (delegate.isEmpty()) {
        delegate = m_delegates.value("");
        if (delegate.isEmpty()) {
            qFatal(QString("Could not find a delegate for %1").arg(videoNode->metaObject()->className()).toLatin1().data());
        }
    }
    auto qmlFileInfo = QFileInfo(QString("../resources/qml/%1.qml").arg(delegate));
    QQmlEngine *engine = QQmlEngine::contextForObject(this)->engine();
    QQmlComponent component(engine, QUrl::fromLocalFile(qmlFileInfo.absoluteFilePath()));
    if( component.status() != QQmlComponent::Ready )
    {
        if(component.status() == QQmlComponent::Error) {
            qDebug() << component.errorString();
            qFatal("Could not construct delegate");
        }
    }
    QSharedPointer<QQuickItem> item(qobject_cast<QQuickItem *>(component.create()));

    item->setParentItem(this);
    QVariant v;
    v.setValue(videoNode);
    item->setProperty("videoNode", v);
    QVariant m;
    m.setValue(m_model);
    item->setProperty("model", m);
    c.item = item;

    return c;
}

static void setInputHeight(const QVector<QVector<int>> &inputs, QVector<QVector<int>> &inputHeight, int node) {
    auto myInputs = inputs.at(node);
    for(int i=0; i<myInputs.count(); i++) {
        int attachedNode = myInputs.at(i);
        if (attachedNode >= 0) {
            setInputHeight(inputs, inputHeight, attachedNode);
            auto childInputHeights = inputHeight.at(attachedNode);
            int myInputHeight = 0;
            for(int j=0; j<childInputHeights.count(); j++) {
                myInputHeight += childInputHeights.at(j);
            }
            inputHeight[node][i] = myInputHeight;
        } else {
            inputHeight[node][i] = 1;
        }
    }
}

static void setLayer(const QVector<QVector<int>> &inputs, QVector<int> &layers, int node, int layer) {
    layers[node] = layer;
    auto myInputs = inputs.at(node);
    for(int i=0; i<myInputs.count(); i++) {
        int attachedNode = myInputs.at(i);
        if (attachedNode >= 0) {
            setLayer(inputs, layers, attachedNode, layer + 1);
        }
    }
}

static void setStackup(const QVector<QVector<int>> &inputs, const QVector<QVector<int>> &inputHeight, QVector<int> &stacks, int node, int stack) {
    stacks[node] = stack;
    auto myInputs = inputs.at(node);
    auto myInputHeights = inputHeight.at(node);
    for(int i=0; i<myInputs.count(); i++) {
        int attachedNode = myInputs.at(i);
        if (attachedNode >= 0) {
            setStackup(inputs, inputHeight, stacks, attachedNode, stack);
        }
        stack += myInputHeights.at(i);
    }
}

void View::onGraphChanged() {
    if (m_model == nullptr) return;

    QMap<VideoNode *, int> existingChildMap;
    for (int i=0; i<m_children.count(); i++) {
        existingChildMap.insert(m_children.at(i).videoNode, i);
    } 

    QList<Child> newChildren;
    auto vertices = m_model->vertices();
    auto edges = m_model->edges();

    for (auto v = vertices.begin(); v != vertices.end(); v++) {
        auto oldChild = existingChildMap.value(*v, -1);
        if (oldChild != -1) {
            newChildren.append(m_children.at(oldChild));
        } else {
            newChildren.append(newChild(*v));
        }
    }

    m_children = newChildren;

    // Create a map from VideoNodes to indices
    QMap<VideoNode *, int> map;
    for (int i=0; i<vertices.count(); i++) {
        map.insert(vertices.at(i), i);
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
    QVector<QVector<int>> inputHeight;
    QVector<int> gridX(vertices.count(), -1);
    QVector<int> gridY(vertices.count(), -1);

    // Create a list of -1's
    for (int i=0; i<vertices.count(); i++) {
        auto inputCount = vertices.at(i)->inputCount();
        inputs.append(QVector<int>(inputCount, -1));
        inputHeight.append(QVector<int>(inputCount, 1));
    }

    for (int i = 0; i < toVertex.count(); i++) {
        auto to = toVertex.at(i);
        if (to >= 0) {
            inputs[to][edges.at(i).toInput] = fromVertex.at(i);
        }
    }

    // Find the root nodes
    QVector<int> s;
    {
        QSet<VideoNode *> sSet;
        // Populate s, the start nodes
        for (int i=0; i<vertices.count(); i++) {
            sSet.insert(vertices.at(i));
        }
        for (auto e = edges.begin(); e != edges.end(); e++) {
            sSet.remove(e->fromVertex);
        }
        for (int i=0; i<vertices.count(); i++) {
            if (sSet.contains(vertices.at(i))) {
                s.append(i);
            }
        }
    }

    int stack = 0;
    for (int i=0; i<s.count(); i++) {
        setInputHeight(inputs, inputHeight, s.at(i));
        setLayer(inputs, gridX, s.at(i), 0);
        setStackup(inputs, inputHeight, gridY, s.at(i), stack);
        auto myInputHeights = inputHeight.at(s.at(i));
        for (int j=0; j<myInputHeights.count(); j++) {
            stack += myInputHeights.at(j);
        }
    }

    for (int i=0; i<m_children.count(); i++) {
        QVariantList v;
        auto myInputHeights = inputHeight.at(i);
        for (int j=0; j<myInputHeights.count(); j++) {
            v.append(myInputHeights.at(j));
        }
        m_children[i].item->setProperty("inputHeights", v);
        m_children[i].item->setProperty("gridX", gridX.at(i));
        m_children[i].item->setProperty("gridY", gridY.at(i));
    }

    QList<QSharedPointer<QQuickItem>> dropAreas;
    for (int i=0; i<m_children.count(); i++) {
        auto myInputHeights = inputHeight.at(i);
        for (int j=0; j<myInputHeights.count(); j++) {
            auto qmlFileInfo = QFileInfo(QString("../resources/qml/TileDropArea.qml"));
            QQmlEngine *engine = QQmlEngine::contextForObject(this)->engine();
            QQmlComponent component(engine, QUrl::fromLocalFile(qmlFileInfo.absoluteFilePath()));
            if( component.status() != QQmlComponent::Ready )
            {
                if(component.status() == QQmlComponent::Error) {
                    qDebug() << component.errorString();
                    qFatal("Could not construct TileDropArea");
                }
            }
            QSharedPointer<QQuickItem> item(qobject_cast<QQuickItem *>(component.create()));

            item->setParentItem(this);
            item->setProperty("gridX", gridX.at(i) + 0.5);
            item->setProperty("gridY", gridY.at(i) + j);
            item->setProperty("gridHeight", myInputHeights.at(j));
            QVariant fromNode;
            fromNode.setValue(static_cast<VideoNode *>(nullptr));

            // TODO this makes this N^2, a clever QMap could solve this
            for (int k=0; k<edges.count(); k++) {
                if (edges.at(k).toVertex == vertices.at(i)
                 && edges.at(k).toInput == j) {
                    fromNode.setValue(edges.at(k).fromVertex);
                    break;
                }
            }

            item->setProperty("fromNode", fromNode);
            QVariant toNode;
            toNode.setValue(vertices.at(i));
            item->setProperty("toNode", toNode);
            item->setProperty("toInput", j);
            dropAreas.append(item);
        }
    }
    m_dropAreas = dropAreas;

    //qDebug() << "vertices" << vertices;
    //qDebug() << "root nodes" << s;
    //qDebug() << "input heights" << inputHeight;
    //qDebug() << "gridX" << gridX;
    //qDebug() << "gridY" << gridY;

    selectionChanged();
}

void View::selectionChanged() {
    QSet<QQuickItem *> found;
    for (int i=0; i<m_children.count(); i++) {
        auto selected = m_selection.contains(m_children.at(i).item.data());
        m_children[i].item->setProperty("selected", selected);
        if (selected) found.insert(m_children.at(i).item.data());
    }
    m_selection = found;
}

Model *View::model() {
    return m_model;
}

void View::setModel(Model *model) {
    if(m_model != nullptr) disconnect(model, nullptr, this, nullptr);
    m_model = model;
    if(m_model != nullptr) {
        connect(model, &Model::videoNodeAdded, this, &View::onGraphChanged);
        connect(model, &Model::videoNodeRemoved, this, &View::onGraphChanged);
        connect(model, &Model::edgeAdded, this, &View::onGraphChanged);
        connect(model, &Model::edgeRemoved, this, &View::onGraphChanged);
    }
    rebuild();
    emit modelChanged(model);
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

void View::select(QVariantList tiles) {
    m_selection.clear();
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<QQuickItem*>(tiles[i]);
        m_selection.insert(tile);
    }
    selectionChanged();
}

void View::addToSelection(QVariantList tiles) {
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<QQuickItem*>(tiles[i]);
        m_selection.insert(tile);
    }
    selectionChanged();
}

void View::removeFromSelection(QVariantList tiles) {
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<QQuickItem*>(tiles[i]);
        m_selection.remove(tile);
    }
    selectionChanged();
}

void View::toggleSelection(QVariantList tiles) {
    bool allSelected = true;
    for (int i=0; i<tiles.count(); i++) {
        auto tile = qvariant_cast<QQuickItem*>(tiles[i]);
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

void View::ensureSelected(QQuickItem *tile) {
    if (!m_selection.contains(tile)) {
        m_selection.clear();
        m_selection.insert(tile);
        selectionChanged();
    }
}

QVariantList View::selection() {
    QVariantList selection;
    for (int i=0; i<m_children.count(); i++) {
        if (m_selection.contains(m_children.at(i).item.data())) {
            selection.append(QVariant::fromValue(m_children.at(i).item.data()));
        }
    }
    return selection;
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
    auto vertices = m_model->vertices();
    QSet<VideoNode *> selectedVerticesSet;
    QVector<VideoNode *> selectedVertices;
    for (int i=0; i<m_children.count(); i++) {
        if (m_selection.contains(m_children.at(i).item.data())) {
            selectedVertices.append(m_children.at(i).videoNode);
            selectedVerticesSet.insert(m_children.at(i).videoNode);
        }
    }

    // Find only the edges contained in the selection
    QVector<Edge> selectedEdges;
    auto edges = m_model->edges();
    for (int i=0; i<edges.count(); i++) {
        if (selectedVerticesSet.contains(edges.at(i).fromVertex)
         && selectedVerticesSet.contains(edges.at(i).toVertex)) {
            selectedEdges.append(edges.at(i));
        }
    }

    // Create a map from VideoNodes to indices
    QMap<VideoNode *, int> map;
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
        edgeRevLookup[i] = QVector<int>(selectedVertices.at(i)->inputCount(), -1);
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
        QSet<VideoNode *> coloredVerticesSet;
        for (int i=0; i<m_children.count(); i++) {
            auto tileIndex = map.value(m_children.at(i).videoNode, -1);
            if (tileIndex >= 0 && colors.at(tileIndex) == c) {
                tiles.append(QVariant::fromValue(m_children.at(i).item.data()));
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
        QSet<VideoNode *> sSet = coloredVerticesSet;
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
