#include "LuxOutputNode.h"

LuxOutputNode::LuxOutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(context, chainSize, 10) {

    connect(this, &SelfTimedReadBackOutputNode::frame, this, &LuxOutputNode::onFrame, Qt::DirectConnection);

    start();
}

LuxOutputNode::LuxOutputNode(const LuxOutputNode &other)
    : SelfTimedReadBackOutputNode(other) {
}

void LuxOutputNode::onFrame(QSize size, QByteArray frame) {
    qDebug() << size << frame;
}

QString LuxOutputNode::typeName() {
    return "LuxOutputNode";
}

VideoNode *LuxOutputNode::deserialize(Context *context, QJsonObject obj) {
    LuxOutputNode *e = new LuxOutputNode(context, QSize(4, 4));
    return e;
}

bool LuxOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *LuxOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> LuxOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("LuxOutput", "LuxOutputInstantiator.qml");
    return m;
}

LuxOutputNode *LuxOutputNode::clone() const {
    return new LuxOutputNode(*this);
}

QSharedPointer<LuxOutputNodePrivate> LuxOutputNode::d() const {
    return d_ptr.staticCast<LuxOutputNodePrivate>();
}

LuxOutputNode::LuxOutputNode(QSharedPointer<LuxOutputNodePrivate> other_ptr)
    : SelfTimedReadBackOutputNode(other_ptr.staticCast<SelfTimedReadBackOutputNodePrivate>())
{
}

LuxOutputNodePrivate::LuxOutputNodePrivate(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNodePrivate(context, chainSize)
{
}
