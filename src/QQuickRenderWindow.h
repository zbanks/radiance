#pragma once

#include <QQuickWindow>

class QQuickRenderWindow : public QQuickWindow {
    Q_OBJECT

public:
    QQuickRenderWindow();
    virtual ~QQuickRenderWindow();

signals:

private:

protected:
    QSharedPointer<RenderContext> m_context;
};
