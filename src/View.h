#pragma once
#include <QQuickItem>
#include "Model.h"

struct Child {
    VideoNode *videoNode;
    QSharedPointer<QQuickItem> item;
    QVector<int> inputHeights;
};

class View : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(Model *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariantMap delegates READ qml_delegates WRITE qml_setDelegates NOTIFY qml_delegatesChanged)

public:
    View();
    ~View() override;

    Model *model();
    void setModel(Model *model);
    QMap<QString, QString> delegates();
    void setDelegates(QMap<QString, QString> delegates);
    QVariantMap qml_delegates();
    void qml_setDelegates(QVariantMap delegates);

public slots:
    void onGraphChanged();

protected:
    Model *m_model;
    QMap<QString, QString> m_delegates;
    QList<Child> m_children;
    void rebuild();
    Child newChild(VideoNode *videoNode);

signals:
    void modelChanged(Model *model);
    void qml_delegatesChanged(QVariantMap delegates);
    void delegatesChanged(QMap<QString, QString> delegates);
};
