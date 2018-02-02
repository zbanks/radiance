#include "ScreenOutputNode.h"
#include <QDebug>
#include <QJsonObject>

ScreenOutputNode::ScreenOutputNode(Context *context, QSize chainSize)
    : OutputNode(context, chainSize)
    , m_outputWindow(nullptr) {
    m_outputWindow = new QWindow();
    m_outputWindow->setGeometry(100, 100, 500, 500);
    m_outputWindow->hide();
}

ScreenOutputNode::ScreenOutputNode(const ScreenOutputNode &other)
    : OutputNode(other) {
}

ScreenOutputNode::~ScreenOutputNode() {
    delete m_outputWindow;
}

void ScreenOutputNode::setVisible(bool visible) {
    m_outputWindow->setVisible(visible);
    emit visibleChanged(visible);
}

bool ScreenOutputNode::visible() {
    return m_outputWindow->isVisible();
}

//////////////////////////////////////////////////////////////////////////////

QString ScreenOutputNodeFactory::typeName() {
    return "ScreenOutputNode";
}

VideoNode *ScreenOutputNodeFactory::deserialize(Context *context, QJsonObject obj) {
    ScreenOutputNode *e = new ScreenOutputNode(context, QSize(500, 500));
    return e;
}
