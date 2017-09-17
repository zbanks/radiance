#pragma once

#include "Output.h"
#include <QQuickItem>

class QQuickOutputItemOutput;

class QQuickOutputItem : public QQuickItem {
    friend class QQuickOutputItemOutput;

    Q_OBJECT
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(Output *output READ output CONSTANT)

public:
    QQuickOutputItem();
   ~QQuickOutputItem() override;

    Output *output();
    void display(GLuint textureId);
    QSize size();
    void setSize(QSize size);

signals:
    void sizeChanged(QSize size);

protected slots:
    void onWindowChanged(QQuickWindow *window);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void updateChain();

    QSharedPointer<QQuickOutputItemOutput> m_output;
    QSize m_size;
    GLuint m_textureId;
    QQuickWindow *m_window;
};
