use std::collections::HashMap;
use crate::node::{NodeId, NodeState};
use crate::graph::Graph;

pub struct Chain {
    node_state: HashMap<NodeId, NodeState>,
}

impl Chain {
    pub fn paint(graph: &Graph, time: f32, chain: &mut Chain) {
    }
}
