use crate::err::Result;
use crate::video_node::{EffectNode, MediaNode, OutputNode, VideoNode, VideoNodeId, VideoNodeType};
use log::*;
use petgraph::graphmap::DiGraphMap;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::borrow::{Borrow, BorrowMut};
use std::collections::HashMap;

/// Directed graph abstraction that owns VideoNodes
/// - Enforces that there are no cycles
/// - Each VideoNode can have up to `node.n_inputs` incoming edges,
///     which must all have unique edge weights in [0..node.n_inputs)
pub struct Graph {
    nodes: HashMap<VideoNodeId, Box<dyn VideoNode>>,
    digraph: DiGraphMap<VideoNodeId, usize>,
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Serialize, Deserialize)]
struct StateEdge {
    #[serde(rename = "fromVertex")]
    from_node: usize,
    #[serde(rename = "toVertex")]
    to_node: usize,
    to_input: usize,
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Deserialize)]
struct StateNode {
    #[serde(rename = "uid")]
    id: Option<VideoNodeId>,
    node_type: VideoNodeType,
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Serialize, Deserialize)]
struct State {
    #[serde(rename = "vertices")]
    nodes: Vec<serde_json::Value>,
    edges: Vec<StateEdge>,
}

impl Graph {
    pub fn new() -> Graph {
        Graph {
            digraph: DiGraphMap::new(),
            nodes: Default::default(),
        }
    }

    pub fn node(&self, id: VideoNodeId) -> Option<&dyn VideoNode> {
        self.nodes.get(&id).map(|n| n.borrow())
    }

    pub fn node_mut(&mut self, id: VideoNodeId) -> Option<&mut (dyn VideoNode + 'static)> {
        self.nodes.get_mut(&id).map(|n| n.borrow_mut())
    }

    pub fn nodes(&self) -> impl Iterator<Item = &dyn VideoNode> {
        self.nodes.values().map(|n| n.borrow())
    }

    pub fn nodes_mut(&mut self) -> impl Iterator<Item = &mut (dyn VideoNode + 'static)> {
        // TODO: How to return Iterator<Item = &mut dyn VideoNode> instead?
        self.nodes.values_mut().map(|n| (*n).borrow_mut())
    }

    pub fn ids(&self) -> impl Iterator<Item = &VideoNodeId> {
        self.nodes.keys()
    }

    pub fn root_nodes(&self) -> Vec<&dyn VideoNode> {
        let mut nodes: Vec<_> = self
            .nodes()
            .filter(|n| {
                self.digraph
                    .neighbors_directed(n.id(), petgraph::Direction::Outgoing)
                    .next()
                    .is_none()
            })
            .collect();
        nodes.sort_by_key(|n| n.id());
        nodes
    }

    pub fn add_videonode<VN: VideoNode + 'static>(&mut self, node: VN) {
        self.digraph.add_node(node.id());
        let n = Box::new(node);
        self.nodes.insert(n.id(), n);
    }

    #[allow(dead_code)]
    pub fn remove_videonode<VN: VideoNode>(&mut self, node: &VN) {
        self.digraph.remove_node(node.id());
        self.nodes.remove(&node.id());
    }

    pub fn add_edge_by_ids(
        &mut self,
        src_id: VideoNodeId,
        dst_id: VideoNodeId,
        input: usize,
    ) -> Result<()> {
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

    pub fn input_for_id(&self, dst_id: VideoNodeId, input: usize) -> Option<VideoNodeId> {
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

    pub fn toposort(&self) -> Vec<&dyn VideoNode> {
        petgraph::algo::toposort(&self.digraph, None)
            .unwrap()
            .iter()
            .map(|id| self.nodes.get(id).unwrap().borrow())
            .collect()
    }

    pub fn node_inputs(&self, node: &dyn VideoNode) -> Vec<Option<&dyn VideoNode>> {
        let mut inputs = Vec::new();
        inputs.resize(node.n_inputs(), None);

        for src_id in self
            .digraph
            .neighbors_directed(node.id(), petgraph::Direction::Incoming)
        {
            let src_index = *self.digraph.edge_weight(src_id, node.id()).unwrap();
            if src_index < node.n_inputs() {
                inputs[src_index] = self.nodes.get(&src_id).map(|n| n.borrow());
            }
        }

        inputs
    }

    pub fn disconnect_node(&mut self, id: VideoNodeId) -> Result<()> {
        let node = self.nodes.get(&id).unwrap();
        let inputs = self.node_inputs(node.borrow());
        let src_id = inputs.first().unwrap_or(&None).map(|n| n.id());
        let src_edges: Vec<VideoNodeId> = inputs.into_iter().flatten().map(|n| n.id()).collect();

        let edges_to_remove: Vec<(VideoNodeId, usize)> = self
            .digraph
            .neighbors_directed(node.id(), petgraph::Direction::Outgoing)
            .map(|dst_id| {
                let dst_index = *self.digraph.edge_weight(node.id(), dst_id).unwrap();
                (dst_id, dst_index)
            })
            .collect();
        for (dst_id, dst_index) in edges_to_remove {
            self.digraph.remove_edge(node.id(), dst_id);
            if let Some(id) = src_id {
                self.digraph.add_edge(id, dst_id, dst_index);
            }
        }
        src_edges.iter().for_each(|src| {
            self.digraph.remove_edge(*src, id);
        });
        self.assert_no_cycles();
        Ok(())
    }

    fn assert_no_cycles(&self) {
        assert!(petgraph::algo::toposort(&self.digraph, None).is_ok());
    }

    pub fn state(&self) -> JsonValue {
        let mut nodes_vec = self.nodes().collect::<Vec<_>>();
        nodes_vec.sort_by_key(|n| n.id());
        let id_map: HashMap<VideoNodeId, usize> = nodes_vec
            .iter()
            .enumerate()
            .map(|(i, n)| (n.id(), i))
            .collect();

        let nodes = nodes_vec.iter().map(|n| n.state()).collect();
        let edges = self
            .digraph
            .all_edges()
            .map(|(a, b, i)| StateEdge {
                from_node: *id_map.get(&a).unwrap(),
                to_node: *id_map.get(&b).unwrap(),
                to_input: *i,
            })
            .collect();
        let state = State { nodes, edges };
        serde_json::to_value(&state).unwrap_or(JsonValue::Null)
    }

    pub fn set_state(&mut self, state: JsonValue) -> Result<()> {
        info!("whole state: {:?}", state);
        let mut state: State = serde_json::from_value(state)?;
        let mut new_graph = DiGraphMap::new();
        self.digraph.clear();
        let mut id_map: HashMap<usize, VideoNodeId> = HashMap::new();
        let mut unseen_ids: HashMap<VideoNodeId, _> = self.nodes().map(|n| (n.id(), ())).collect();
        let mut nodes_to_insert: Vec<Box<dyn VideoNode>> = vec![];

        // Lookup or create all of the nodes
        for (i, s) in state.nodes.drain(0..).enumerate() {
            let n: StateNode = serde_json::from_value(s.clone())?;
            let (id, node) = if let Some(id) = n.id {
                // UID was specified, so find the node
                let node = self.nodes.get_mut(&id).ok_or("Invalid node ID")?;
                (id, node)
            } else {
                // The node doesn't exist, so create it
                let boxed_node: Box<dyn VideoNode> = match n.node_type {
                    VideoNodeType::Effect => Box::new(EffectNode::new()?),
                    VideoNodeType::Output => Box::new(OutputNode::new()),
                    VideoNodeType::Media => Box::new(MediaNode::new()),
                };
                let id = boxed_node.id();
                info!("Added node type {:?} with id {:?}", n.node_type, id);
                nodes_to_insert.push(boxed_node);
                (id, nodes_to_insert.last_mut().unwrap())
            };

            node.set_state(s)?;
            new_graph.add_node(id);
            id_map.insert(i, id);
            unseen_ids.remove(&id);
        }

        // Calculate the new set of edges
        for edge in state.edges {
            new_graph.add_edge(
                *id_map.get(&edge.from_node).ok_or("invalid from_node")?,
                *id_map.get(&edge.to_node).ok_or("invalid to_node")?,
                edge.to_input,
            );
        }

        // Remove nodes not referenced
        for (id, _) in unseen_ids {
            self.nodes.remove(&id);
        }

        // Add newly created nodes
        for node in nodes_to_insert {
            let id = node.id();
            self.nodes.insert(id, node);
        }
        self.digraph = new_graph;
        self.assert_no_cycles();

        Ok(())
    }
}
