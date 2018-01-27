#include "OutputNode.h"
#include "ProbeReg.h"
#include <QDebug>
#include "main.h"

OutputType::OutputType(NodeRegistry *r , QObject *p )
    : NodeType(r,p) {
}

OutputType::~OutputType() = default;

VideoNode *OutputType::create(QString arg) {
    auto node = new OutputNode(this);
    if (node) {
        node->setInputCount(inputCount());
    }
    return node;
}

OutputNode::OutputNode(NodeType *nr)
    : VideoNode(nr) {
}

OutputNode::OutputNode(const OutputNode &other)
    : VideoNode(other) {
}

OutputNode::~OutputNode() = default;

QString OutputNode::serialize() {
    return "its_an_output";
}

void OutputNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
}

QSharedPointer<VideoNode> OutputNode::createCopyForRendering(QSharedPointer<Chain> chain) {
    Q_UNUSED(chain);
    return QSharedPointer<VideoNode>(new OutputNode(*this));
}

GLuint OutputNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    Q_UNUSED(chain);
    return inputTextures.at(0);
}

namespace {
std::once_flag reg_once{};
TypeRegistry output_registry{[](NodeRegistry *r) -> QList<NodeType*> {
    std::call_once(reg_once,[](){
        qmlRegisterUncreatableType<OutputNode>("radiance",1,0,"OutputNode","OutputNode must be created through the registry");
    });
    auto res = QList<NodeType*>{};

    auto t = new OutputType(r);
    t->setName("Output");
    t->setInputCount(1);
    res.append(t);

    return res;
}};
}

