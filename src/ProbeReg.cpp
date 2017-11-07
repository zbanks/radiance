#include "ProbeReg.h"
#include "VideoNode.h"
#include "NodeType.h"
#include "NodeRegistry.h"

std::atomic<ProbeRegistry *> ProbeRegistry::s_head{};

const ProbeRegistry *ProbeRegistry::head() {
    return s_head.load();
}

QList<NodeType *> ProbeRegistry::probeAll(NodeRegistry * nr) {
    auto res = QList<NodeType*>{};
    for (auto r = head(); r; r = r->next()) {
        res.append(r->probe(nr));
    }
    return res;
}

QList<NodeType *> ProbeRegistry::probe(NodeRegistry *) const {
    return {};
}

const ProbeRegistry *ProbeRegistry::next() const {
    return m_next.load();
}

ProbeRegistry *ProbeRegistry::next() {
    return m_next.load();
}

ProbeRegistry::ProbeRegistry()
    : m_next(s_head.exchange(this)) {
}
