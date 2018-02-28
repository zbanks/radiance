#include "ConsoleOutputNode.h"

ConsoleOutputNode::ConsoleOutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(context, chainSize) {
}

ConsoleOutputNode::ConsoleOutputNode(const ConsoleOutputNode &other)
    : SelfTimedReadBackOutputNode(other) {
}

ConsoleOutputNode::~ConsoleOutputNode() {
}

void ConsoleOutputNode::frame(QSize size, QByteArray *frame) {
    qDebug() << size << *frame;
}

QString ConsoleOutputNode::typeName() {
    return "ConsoleOutputNode";
}

VideoNode *ConsoleOutputNode::deserialize(Context *context, QJsonObject obj) {
    ConsoleOutputNode *e = new ConsoleOutputNode(context, QSize(3, 3));
    return e;
}

bool ConsoleOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *ConsoleOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> ConsoleOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("ConsoleOutput", "ConsoleOutputInstantiator.qml");
    return m;
}
