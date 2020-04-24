use crate::err::Result;
use crate::video_node::VideoNode;
use log::*;
use petgraph::graphmap::DiGraphMap;
use std::collections::HashMap;

/// Directed graph abstraction that owns VideoNodes
/// - Enforces that there are no cycles
/// - Each VideoNode can have up to `node.n_inputs` incoming edges,
///     which must all have unique edge weights in [0..node.n_inputs)
pub struct Graph {
    nodes: HashMap<usize, VideoNode>,
    digraph: DiGraphMap<usize, usize>,
}

impl Graph {
    pub fn new() -> Graph {
        Graph {
            digraph: DiGraphMap::new(),
            nodes: Default::default(),
        }
    }

    pub fn node(&self, id: usize) -> Option<&VideoNode> {
        self.nodes.get(&id)
    }

    pub fn node_mut(&mut self, id: usize) -> Option<&mut VideoNode> {
        self.nodes.get_mut(&id)
    }

    pub fn nodes(&self) -> impl Iterator<Item = &VideoNode> {
        self.nodes.values()
    }

    pub fn nodes_mut(&mut self) -> impl Iterator<Item = &mut VideoNode> {
        self.nodes.values_mut()
    }

    pub fn ids(&self) -> impl Iterator<Item = &usize> {
        self.nodes.keys()
    }

    pub fn root_nodes(&self) -> Vec<&VideoNode> {
        let mut nodes: Vec<_> = self
            .nodes()
            .filter(|n| {
                self.digraph
                    .neighbors_directed(n.id, petgraph::Direction::Outgoing)
                    .next()
                    .is_none()
            })
            .collect();
        nodes.sort_by_key(|n| n.id);
        nodes
    }

    pub fn add_videonode(&mut self, node: VideoNode) {
        self.digraph.add_node(node.id);
        self.nodes.insert(node.id, node);
    }

    #[allow(dead_code)]
    pub fn remove_videonode(&mut self, node: &VideoNode) {
        self.digraph.remove_node(node.id);
        self.nodes.remove(&node.id);
    }

    pub fn add_edge_by_ids(&mut self, src_id: usize, dst_id: usize, input: usize) -> Result<()> {
        // TODO: safety check
        if src_id == dst_id {
            return Err("Adding self edge would cause cycle".into());
        }
        if let Some(old_src_id) = self.input_for_id(dst_id, input) {
            self.digraph.remove_edge(old_src_id, dst_id);
            self.digraph.add_edge(old_src_id, src_id, 0);
        }
        self.digraph.add_edge(src_id, dst_id, input);
        self.assert_no_cycles();
        Ok(())
    }

    pub fn input_for_id(&self, dst_id: usize, input: usize) -> Option<usize> {
        for src_id in self
            .digraph
            .neighbors_directed(dst_id, petgraph::Direction::Incoming)
        {
            if *self.digraph.edge_weight(src_id, dst_id).unwrap() == input {
                return Some(src_id);
            }
        }
        None
    }

    pub fn toposort(&self) -> Vec<&VideoNode> {
        petgraph::algo::toposort(&self.digraph, None)
            .unwrap()
            .iter()
            .map(|id| self.nodes.get(id).unwrap())
            .collect()
    }

    pub fn node_inputs(&self, node: &VideoNode) -> Vec<Option<&VideoNode>> {
        let mut inputs = Vec::new();
        inputs.resize(node.n_inputs, None);

        for src_id in self
            .digraph
            .neighbors_directed(node.id, petgraph::Direction::Incoming)
        {
            let src_index = *self.digraph.edge_weight(src_id, node.id).unwrap();
            if src_index < node.n_inputs {
                inputs[src_index] = self.nodes.get(&src_id);
            }
        }

        inputs
    }

    pub fn disconnect_node(&mut self, id: usize) -> Result<()> {
        // XXX There is a bug here
        let node = self.nodes.get(&id).unwrap();
        let src_id = self
            .node_inputs(node)
            .first()
            .unwrap_or(&None)
            .map(|n| n.id);
        let edges_to_remove: Vec<(usize, usize)> = self
            .digraph
            .neighbors_directed(node.id, petgraph::Direction::Outgoing)
            .map(|dst_id| {
                let dst_index = *self.digraph.edge_weight(node.id, dst_id).unwrap();
                (dst_id, dst_index)
            })
            .collect();
        for (dst_id, dst_index) in edges_to_remove {
            info!(
                "disconnect {} remove -> {}; add {:?} -> {}, {}",
                node.id, dst_id, src_id, dst_id, dst_index
            );
            self.digraph.remove_edge(node.id, dst_id);
            if let Some(id) = src_id {
                self.digraph.add_edge(id, dst_id, dst_index);
            }
        }
        self.assert_no_cycles();
        Ok(())
    }

    fn assert_no_cycles(&self) {
        petgraph::algo::toposort(&self.digraph, None).unwrap();
    }
}
