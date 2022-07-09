use std::collections::HashMap;
use crate::node::{NodeId, NodeState};
use crate::graph::Graph;
use crate::chain::Chain;

pub struct Context {
    nodes: HashMap<NodeId, NodeState>,
}

impl Context {
    pub fn paint(graph: &Graph, chain: &mut Chain, time: f32) {
    }
}
