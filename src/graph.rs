use std::collections::{HashMap, HashSet};
use crate::node::{NodeId, NodeArgs, NodeProps};

pub struct Graph {
    nodes: HashMap<NodeId, (NodeArgs, NodeProps)>,
    edges: HashSet<(NodeId, NodeId)>,
}
