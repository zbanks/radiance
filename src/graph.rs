use crate::effect_node::EffectNodeProps;
use rand::Rng;
use std::collections::{HashMap, HashSet};

#[derive(Eq, Hash, PartialEq, Debug, Clone, Copy)]
pub struct NodeId(u128);

impl NodeId {
    pub fn gen() -> NodeId {
        NodeId(rand::thread_rng().gen())
    }
}

pub enum NodeProps {
    /// Node props are construction arguments and
    /// per-node rendering inputs
    /// that are passed in every frame
    /// in a rest-ful style.

    EffectNode(EffectNodeProps),
}

pub struct Graph {
    pub nodes: HashMap<NodeId, NodeProps>,
    pub edges: HashSet<(NodeId, NodeId)>,
}

impl Graph {
    pub fn new() -> Self {
        Self {
            nodes: HashMap::new(),
            edges: HashSet::new(),
        }
    }

    pub fn nodes(&self) -> &HashMap<NodeId, NodeProps> {
        &self.nodes
    }

    pub fn insert_node(&mut self, id: NodeId, props: NodeProps) {
        assert!(!self.nodes.contains_key(&id), "Given node ID already exists in graph");
        self.nodes.insert(id, props);
    }
}
