#include "NodeType.h"
#include "VideoNode.h"

NodeType::NodeType(QObject *p)
: QObject(p)
{
}
NodeType::~NodeType() = default;

QString NodeType::name() const
{
    return m_name;
}
QString NodeType::description() const
{
    return m_description;
}
QString NodeType::author() const
{
    return m_author;
}
int NodeType::inputCount() const
{
    return m_inputCount;
}
bool NodeType::ready() const
{
    return m_ready;
}
void NodeType::setName(QString n)
{
    if(n != name()) {
        m_name = n;
        emit nameChanged(n);
    }
}
void NodeType::setDescription(QString n)
{
    if(n != description()) {
        m_description = n;
        emit descriptionChanged(n);
    }
}
void NodeType::setAuthor(QString n)
{
    if(n != author()) {
        m_author= n;
        emit authorChanged(n);
    }
}
void NodeType::isReady()
{
    if(!ready()) {
        m_ready = true;
        emit readyChanged();
    }
}
void NodeType::setInputCount(int n)
{
    if(n != inputCount()) {
        m_inputCount = n;
        emit inputCountChanged(n);
    }
}
VideoNode *NodeType::create(QString arg)
{
    Q_UNUSED(arg);
    return nullptr;
}
