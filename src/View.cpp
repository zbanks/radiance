#include "View.h"
#include "main.h"
#include <QQmlContext>
#include <QQmlProperty>

View::View()
    : m_model(nullptr)
{
    setFlag(ItemHasContents, true);
}

View::~View() {
}

void View::rebuild() {
    m_children.clear();
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
    auto qmlFile = QString("../resources/qml/%1.qml").arg(delegate);
    QQmlEngine *engine = QQmlEngine::contextForObject(this)->engine();
    QQmlComponent component(engine, QUrl::fromLocalFile(qmlFile));
    QSharedPointer<QQuickItem> item(qobject_cast<QQuickItem *>(component.create()));

    item->setParentItem(this);
    QVariant v;
    v.setValue(videoNode);
    item->setProperty("videoNode", v);
    c.item = item;

    return c;
}

void View::onGraphChanged() {
    if (m_model == nullptr) return;

    QMap<VideoNode *, int> map;
    for (int i=0; i<m_children.count(); i++) {
        map.insert(m_children.at(i).videoNode, i);
    } 

    QList<Child> newChildren;
    auto vertices = m_model->vertices();

    for (auto v = vertices.begin(); v != vertices.end(); v++) {
        auto oldChild = map.value(*v, -1);
        if (oldChild != -1) {
            newChildren.append(m_children.at(oldChild));
        } else {
            newChildren.append(newChild(*v));
        }
    }

    m_children = newChildren;

    // Arrange the items
    for (int i=0; i<m_children.count(); i++) {
        m_children[i].item->setX(i * 100);
    }
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
    // Connect signals from model to view here
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
