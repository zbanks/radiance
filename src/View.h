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

    // Selection
    void select(QVariantList tiles);
    void addToSelection(QVariantList tiles);
    void removeFromSelection(QVariantList tiles);
    void toggleSelection(QVariantList tiles);
    QVariantList selection();

protected:
    Model *m_model;
    QMap<QString, QString> m_delegates;
    QList<Child> m_children;
    QList<QSharedPointer<QQuickItem>> m_dropAreas;
    void rebuild();
    Child newChild(VideoNode *videoNode);
    QSet<QQuickItem *> m_selection;
    void selectionChanged();

signals:
    void modelChanged(Model *model);
    void qml_delegatesChanged(QVariantMap delegates);
    void delegatesChanged(QMap<QString, QString> delegates);
};
