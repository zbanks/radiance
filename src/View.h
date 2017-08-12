#pragma once
#include <QQuickItem>
#include "Model.h"

class View : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(Model *model READ model WRITE setModel NOTIFY modelChanged)

public:
    View();
    ~View() override;

    Model *model();
    void setModel(Model *model);

protected:
    Model *m_model;
    QList<QSharedPointer<QQuickItem>> m_children;
    void rebuild();

signals:
    void modelChanged(Model *model);
};
