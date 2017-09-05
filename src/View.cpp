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
