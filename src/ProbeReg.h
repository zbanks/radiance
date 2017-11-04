#pragma once

#include "NodeRegistry.h"
#include "NodeType.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <QList>

class ProbeRegistry {
    static std::atomic<ProbeRegistry *> s_head;
    std::atomic<ProbeRegistry *> m_next{};

public:
    static const ProbeRegistry *head();
    static QList<NodeType *> probeAll(NodeRegistry * nr);
    virtual QList<NodeType *> probe(NodeRegistry *) const;
    const ProbeRegistry *next() const;
    ProbeRegistry *next();
    ProbeRegistry();
};
class TypeRegistry : public ProbeRegistry {
    using fn_type = std::function<QList<NodeType*>(NodeRegistry*)>;
    fn_type m_f;
public:
    template<class F>
    TypeRegistry(F && f)
    : ProbeRegistry()
    , m_f(std::forward<F>(f))
    { }
    QList<NodeType*> probe(NodeRegistry * nr) const override
    {
        return m_f(nr);
    }
};
