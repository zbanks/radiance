use crate::err::{Error, Result};
use crate::video_node::{
    DetailLevel, EffectNode, MediaNode, OutputNode, VideoNode, VideoNodeId, VideoNodeType,
};
use log::*;
use petgraph::graphmap::DiGraphMap;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::borrow::{Borrow, BorrowMut};
use std::collections::{BTreeMap, HashMap, BTreeSet};

struct Graph<T: Copy + Eq + Ord + std::fmt::Debug> {
    /// Map of to_vertex -> [from_vertex_0, from_vertex_1, ...]
    edges: BTreeMap<T, Vec<Option<T>>>,
}

#[allow(dead_code,unused_variables)]
impl<T: Copy + Eq + Ord + std::fmt::Debug> Graph<T> {
    pub fn new() -> Graph<T> {
        Graph {
            edges: Default::default(),
        }
    }

    /// Completely remove all graph state
    pub fn clear(&mut self) {
        self.edges.clear();
    }

    /// Add, modify, or remove a vertex
    /// If `n_inputs` is `None`, remove the vertex and any associated edges
    pub fn set_vertex(&mut self, vertex: T, n_inputs: Option<usize>) {
        if let Some(n_inputs) = n_inputs {
            self.edges.entry(vertex).or_default().resize(n_inputs, None);
        } else {
            self.edges.remove(&vertex);

            // Remove all edges where `vertex` was the `from_vertex`
            for from_vertexes in self.edges.values_mut() {
                for from_vertex in from_vertexes.iter_mut() {
                    if *from_vertex == Some(vertex) {
                        *from_vertex = None
                    }
                }
            }
        }
    }

    /// Add, modify, or remove an edge
    /// If `to_input` is `None`, remove the edge
    pub fn set_edge(&mut self, from_vertex: Option<T>, to_vertex: T, to_input: usize) -> Result<()> {
        // Check that both verticies in the proposed edge exist
        if let Some(fv) = from_vertex {
            if !self.edges.contains_key(&fv) {
                return Err(format!("from_vertex {:?} not in graph", fv).as_str().into());
            }
        }
        if !self.edges.contains_key(&to_vertex) {
            return Err(format!("to_vertex {:?} not in graph", to_vertex).as_str().into());
        }

        // TODO validation that this won't create a cycle

        // Remove the previous edge that went to `(to_vertex, to_input)` & insert from_vertex
        let inputs = self.edges.get_mut(&to_vertex).unwrap();
        *inputs.get_mut(to_input).ok_or("invalid to_input; too large")? = from_vertex;

        Ok(())
    }

    /// Return a Vec of verticies in topological order
    pub fn toposort(&self) -> Result<Vec<T>> {
        let mut rev_edges: BTreeMap<T, BTreeSet<T>> = self.edges.keys().map(|v| (*v, Default::default())).collect();
        for (to_vertex, inputs) in &self.edges {
            for from_vertex in inputs {
                if let Some(from_vertex) = from_vertex {
                    rev_edges.get_mut(from_vertex).unwrap().insert(*to_vertex);
                }
            }
        }
        println!("_edges: {:?}", self.edges);
        println!("rev_edges: {:?}", rev_edges);

        // Kahn's Algorithm from Wikipedia's pseudocode
        // https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm
        // NB: The direction is inverted from the pseudocode description
        //
        // L ← Empty list that will contain the sorted elements
        let mut l: Vec<T> = Default::default();
        // S ← Set of all nodes with no incoming edge
        let mut s: Vec<T> = rev_edges.iter().filter_map(|(v, outs)| if outs.is_empty() { Some(*v) } else { None }).collect();

        // while S is non-empty do
        //     remove a node n from S
        while let Some(n) = s.pop() {
            // add n to tail of L
            l.push(n);
            // for each node m with an edge e from n to m do
            for m in self.edges.get(&n).unwrap().iter().filter_map(|x| x.as_ref()) {
                // remove edge e from the graph
                let m_edges = rev_edges.get_mut(m).unwrap();
                m_edges.remove(&n);
                // if m has no other incoming edges then
                if m_edges.is_empty() {
                    // insert m into S
                    s.push(*m);
                }
            }
        }
        // if graph has edges then
        if !rev_edges.values().all(|outs| outs.is_empty()) {
            // return error   (graph has at least one cycle)
            println!("{:?}", rev_edges);
            //Err("Cycle detected".into())
            Ok(l)
        // else 
        } else {
            // return L   (a topologically sorted order)
            Ok(l)
        }
    }

    /// Return a Vec of input edges for a given vertex
    pub fn vertex_inputs(&self, vertex: T) -> Result<&Vec<Option<T>>> {
        self.edges.get(&vertex).ok_or_else(|| format!("Invalid vertex {:?}", vertex).as_str().into())
    }

    /// Check for cycles or other malformed representations
    /// This is intended to be a debugging tool; Graph should never intentionally
    /// hold malformed or cyclic graphs.
    fn validate(&self) -> Result<()> {
        // TODO
        self.toposort().map(|_| ())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn graph_toposort() {
        let mut g: Graph<usize> = Graph::new();
        g.set_vertex(0, Some(2));
        g.set_vertex(1, Some(2));
        g.set_vertex(2, Some(2));
        g.set_vertex(3, Some(2));
        g.set_vertex(4, Some(2));
        assert_eq!(g.toposort().unwrap(), vec![4, 3, 2, 1, 0]);

        g.set_edge(Some(0), 1, 0).unwrap();
        g.set_edge(Some(1), 2, 0).unwrap();
        g.set_edge(Some(2), 4, 0).unwrap();
        g.set_edge(Some(4), 3, 0).unwrap();
        assert_eq!(g.toposort().unwrap(), vec![3, 4, 2, 1, 0]);

        g.set_edge(Some(0), 3, 0).unwrap();
        assert_eq!(g.toposort().unwrap(), vec![4, 2, 1, 3, 0]);

        g.set_edge(Some(3), 4, 0).unwrap();
        g.set_edge(Some(1), 4, 1).unwrap();
        assert_eq!(g.vertex_inputs(0).unwrap(), &vec![None, None]);
        assert_eq!(g.vertex_inputs(1).unwrap(), &vec![Some(0), None]);
        assert_eq!(g.vertex_inputs(2).unwrap(), &vec![Some(1), None]);
        assert_eq!(g.vertex_inputs(3).unwrap(), &vec![Some(0), None]);
        assert_eq!(g.vertex_inputs(4).unwrap(), &vec![Some(3), Some(1)]);
        assert_eq!(g.toposort().unwrap(), vec![4, 3, 2, 1, 0]);
    }
}

/// Directed graph abstraction that owns VideoNodes
/// - Enforces that there are no cycles
/// - Each VideoNode can have up to `node.n_inputs` incoming edges,
///     which must all have unique edge weights in [0..node.n_inputs)
pub struct Model {
    nodes: HashMap<VideoNodeId, Box<dyn VideoNode>>,
    graph: DiGraphMap<VideoNodeId, usize>,
    dirty: bool,
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
    #[serde(rename = "newVertices", default)]
    node_ids: Vec<VideoNodeId>,
    edges: Vec<StateEdge>,
}

impl Model {
    pub fn new() -> Model {
        Model {
            graph: DiGraphMap::new(),
            nodes: Default::default(),
            dirty: false,
        }
    }

    pub fn node(&self, id: VideoNodeId) -> Option<&dyn VideoNode> {
        self.nodes.get(&id).map(|n| n.borrow())
    }

    #[allow(dead_code)]
    pub fn node_mut(&mut self, id: VideoNodeId) -> Option<&mut (dyn VideoNode + 'static)> {
        self.nodes.get_mut(&id).map(|n| n.borrow_mut())
    }

    pub fn nodes(&self) -> impl Iterator<Item = &dyn VideoNode> {
        self.nodes.values().map(|n| n.borrow())
    }

    pub fn nodes_mut(&mut self) -> impl Iterator<Item = &mut (dyn VideoNode + 'static)> {
        self.nodes.values_mut().map(|n| (*n).borrow_mut())
    }

    pub fn ids(&self) -> impl Iterator<Item = &VideoNodeId> {
        self.nodes.keys()
    }

    pub fn toposort(&self) -> Vec<&dyn VideoNode> {
        petgraph::algo::toposort(&self.graph, None)
            .unwrap()
            .iter()
            .map(|id| self.nodes.get(id).unwrap().borrow())
            .collect()
    }

    pub fn node_inputs(&self, node: &dyn VideoNode) -> Vec<Option<&dyn VideoNode>> {
        let mut inputs = Vec::new();
        inputs.resize(node.n_inputs(), None);

        for src_id in self
            .graph
            .neighbors_directed(node.id(), petgraph::Direction::Incoming)
        {
            let src_index = *self.graph.edge_weight(src_id, node.id()).unwrap();
            if src_index < node.n_inputs() {
                inputs[src_index] = self.nodes.get(&src_id).map(|n| n.borrow());
            }
        }

        inputs
    }

    fn digraph_check_cycles(graph: &DiGraphMap<VideoNodeId, usize>) -> Result<()> {
        if petgraph::algo::toposort(graph, None).is_err() {
            Err(Error::new("Cycle detected"))
        } else {
            Ok(())
        }
    }

    pub fn add_node(&mut self, state: JsonValue) -> Result<VideoNodeId> {
        let node_state: StateNode = serde_json::from_value(state.clone())?;
        let mut boxed_node: Box<dyn VideoNode> = match node_state.node_type {
            VideoNodeType::Effect => Box::new(EffectNode::new()?),
            VideoNodeType::Output => Box::new(OutputNode::new()),
            VideoNodeType::Media => Box::new(MediaNode::new()?),
        };
        boxed_node.set_state(state)?;

        let id = boxed_node.id();
        self.graph.add_node(id);
        self.nodes.insert(id, boxed_node);
        self.dirty = true;

        Ok(id)
    }

    pub fn remove_node(&mut self, id: VideoNodeId) -> Result<()> {
        self.graph.remove_node(id);
        self.nodes.remove(&id);
        self.dirty = true;
        Ok(())
    }

    pub fn clear(&mut self) {
        self.nodes.clear();
        self.graph.clear();
        self.dirty = true;
    }

    pub fn add_edge(&mut self, from: VideoNodeId, to: VideoNodeId, input: usize) -> Result<()> {
        // TODO: Enforce that the new graph is valid
        self.graph.add_edge(from, to, input);
        self.dirty = true;
        Ok(())
    }

    pub fn remove_edge(&mut self, from: VideoNodeId, to: VideoNodeId, _input: usize) -> Result<()> {
        // TODO: Check harder if the old edge existed
        self.graph
            .remove_edge(from, to)
            .ok_or("edge does not exist")?;
        self.dirty = true;
        Ok(())
    }

    pub fn state(&self) -> JsonValue {
        let mut nodes_vec = self.nodes().collect::<Vec<_>>();
        nodes_vec.sort_by_key(|n| n.id());
        let node_ids = nodes_vec.iter().map(|n| n.id()).collect();
        let id_map: HashMap<VideoNodeId, usize> = nodes_vec
            .iter()
            .enumerate()
            .map(|(i, n)| (n.id(), i))
            .collect();

        let nodes = nodes_vec
            .iter()
            .map(|n| n.state(DetailLevel::All))
            .collect();
        let edges = self
            .graph
            .all_edges()
            .map(|(a, b, i)| StateEdge {
                from_node: *id_map.get(&a).unwrap(),
                to_node: *id_map.get(&b).unwrap(),
                to_input: *i,
            })
            .collect();
        let state = State {
            nodes,
            node_ids,
            edges,
        };
        serde_json::to_value(&state).unwrap_or(JsonValue::Null)
    }

    /// Deprecated
    pub fn set_state(&mut self, state: JsonValue) -> Result<()> {
        let mut state: State = serde_json::from_value(state)?;
        let mut new_graph = DiGraphMap::new();
        self.graph.clear();
        let mut id_map: HashMap<usize, VideoNodeId> = HashMap::new();
        let mut unseen_ids: HashMap<VideoNodeId, _> = self.ids().map(|id| (*id, ())).collect();
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
                    VideoNodeType::Media => Box::new(MediaNode::new()?),
                };
                let id = boxed_node.id();
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
        Self::digraph_check_cycles(&new_graph)?;

        // Remove nodes not referenced
        for (id, _) in unseen_ids {
            self.nodes.remove(&id);
        }

        // Add newly created nodes
        for node in nodes_to_insert {
            let id = node.id();
            self.nodes.insert(id, node);
        }
        self.graph = new_graph;

        Ok(())
    }

    pub fn flush(&mut self) -> bool {
        let old_dirty = self.dirty;
        info!("Flushing: {}", old_dirty);
        self.dirty = false;

        old_dirty
    }
}
