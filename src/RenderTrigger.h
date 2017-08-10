#pragma once
#include <QObject>

class RenderContext;
class Model;

class RenderTrigger : public QObject {
    Q_OBJECT

public:
    RenderTrigger(RenderContext *context, Model *model, int chain, QObject *obj);
    RenderTrigger(const RenderTrigger&);
   ~RenderTrigger();
    bool operator==(const RenderTrigger &other) const;
    RenderTrigger& operator=(const RenderTrigger&);
public slots:
    void render();
private:
    RenderContext *m_context;
    int m_chain;
    Model *m_model;
    QObject *m_obj; // Object is only needed for equality check in removeRenderTrigger
};

