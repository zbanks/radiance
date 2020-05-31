use crate::err::{Error, Result};
use crate::library::Library;
use crate::video_node::{IVideoNode, VideoNode, VideoNodeId};
use log::*;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::borrow::{Borrow, BorrowMut};
use std::cell::RefCell;
use std::collections::{BTreeMap, BTreeSet, HashMap};

// Graph is absracted over T to facilitate testing; but is practically for VideoNodeId
#[derive(Serialize, Deserialize)]
#[serde(transparent)]
struct Graph<T: Ord> {
    /// Map of to_vertex -> [from_vertex_0, from_vertex_1, ...]
    edges: BTreeMap<T, Vec<Option<T>>>,
    #[serde(skip, default = "empty_refcell")]
    toposort_cache: RefCell<Option<Vec<T>>>,
}

fn empty_refcell<T>() -> RefCell<Option<T>> {
    RefCell::new(None)
}

#[allow(dead_code, unused_variables)]
impl<T: Copy + Ord + Eq + std::fmt::Debug> Graph<T> {
    pub fn new() -> Graph<T> {
        Graph {
            edges: Default::default(),
            toposort_cache: RefCell::new(None),
        }
    }

    /// Completely remove all graph state
    pub fn clear(&mut self) {
        self.edges.clear();
        self.toposort_cache.replace(None);
    }

    /// Add, modify, or remove a vertex
    /// If `n_inputs` is `None`, remove the vertex and any associated edges
    pub fn set_vertex(&mut self, vertex: T, n_inputs: Option<usize>) {
        if self.edges.get(&vertex).map(|x| x.len()) == n_inputs {
            return;
        }

        self.toposort_cache.replace(None);
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

    /// Test if there is a path from one vertex to another
    /// Used in checking if adding a new edge would create a cycle
    pub fn path_exists(&self, from_vertex: T, to_vertex: T) -> bool {
        if from_vertex == to_vertex {
            true
        } else {
            // XXX: Re-write to avoid recursion
            self.edges
                .get(&to_vertex)
                .unwrap()
                .iter()
                .filter_map(|fv| *fv)
                .any(|fv| self.path_exists(from_vertex, fv))
        }
    }

    /// Add, modify, or remove an edge
    /// If `to_input` is `None`, remove the edge
    pub fn set_edge(
        &mut self,
        from_vertex: Option<T>,
        to_vertex: T,
        to_input: usize,
    ) -> Result<()> {
        self.toposort_cache.replace(None);
        // Check that both verticies in the proposed edge exist
        if !self.edges.contains_key(&to_vertex) {
            return Err(format!("to_vertex {:?} not in graph", to_vertex)
                .as_str()
                .into());
        }
        if let Some(fv) = from_vertex {
            if !self.edges.contains_key(&fv) {
                return Err(format!("from_vertex {:?} not in graph", fv).as_str().into());
            }
            // Validate that this edge won't create a cycle
            if self.path_exists(to_vertex, fv) {
                return Err(Error::GraphCycle);
            }
        }

        // Remove the previous edge that went to `(to_vertex, to_input)` & insert from_vertex
        let inputs = self.edges.get_mut(&to_vertex).unwrap();
        *inputs
            .get_mut(to_input)
            .ok_or("invalid to_input; too large")? = from_vertex;

        Ok(())
    }

    /// Return a Vec with every edge: (from, to, input)
    pub fn all_edges(&self) -> impl Iterator<Item = (T, T, usize)> + '_ {
        self.edges.iter().flat_map(|(to, inputs)| {
            inputs
                .iter()
                .enumerate()
                .filter_map(move |(i, from)| from.map(|f| (f, *to, i)))
        })
    }

    /// Return a Vec of verticies in topological order
    pub fn toposort(&self) -> Result<Vec<T>> {
        if let Some(cache) = &*self.toposort_cache.borrow() {
            return Ok(cache.clone());
        }

        let mut rev_edges: BTreeMap<T, BTreeSet<T>> = self
            .edges
            .keys()
            .map(|v| (*v, Default::default()))
            .collect();
        for (to_vertex, inputs) in &self.edges {
            for from_vertex in inputs {
                if let Some(from_vertex) = from_vertex {
                    rev_edges.get_mut(from_vertex).unwrap().insert(*to_vertex);
                }
            }
        }
        let mut fwd_edges: BTreeMap<T, BTreeSet<T>> = self
            .edges
            .iter()
            .map(|(to, inputs)| (*to, inputs.iter().flatten().copied().collect()))
            .collect();

        // Kahn's Algorithm from Wikipedia's pseudocode
        // https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm
        //
        // L ← Empty list that will contain the sorted elements
        let mut l: Vec<T> = Default::default();
        // S ← Set of all nodes with no incoming edge
        let mut s: Vec<T> = fwd_edges
            .iter()
            .filter_map(|(v, outs)| if outs.is_empty() { Some(*v) } else { None })
            .collect();

        // while S is non-empty do
        //     remove a node n from S
        while let Some(n) = s.pop() {
            // add n to tail of L
            l.push(n);
            // for each node m with an edge e from n to m do
            for m in rev_edges.get(&n).unwrap().clone().iter() {
                // remove edge e from the graph
                rev_edges.get_mut(&n).unwrap().remove(&m);
                let m_edges = fwd_edges.get_mut(m).unwrap();
                m_edges.remove(&n);
                // if m has no other incoming edges then
                if m_edges.is_empty() {
                    // insert m into S
                    s.push(*m);
                }
            }
        }
        // if graph has edges then
        if !fwd_edges.values().all(|outs| outs.is_empty()) {
            // return error   (graph has at least one cycle)
            info!("Cycle found: {:?}", fwd_edges);
            Err("Cycle detected".into())
        // else
        } else {
            // return L   (a topologically sorted order)
            self.toposort_cache.replace(Some(l.clone()));
            Ok(l)
        }
    }

    /// Return a Vec of input edges for a given vertex
    pub fn vertex_inputs(&self, vertex: T) -> Result<&Vec<Option<T>>> {
        self.edges
            .get(&vertex)
            .ok_or_else(|| format!("Invalid vertex {:?}", vertex).as_str().into())
    }

    /// Check for cycles or other malformed representations
    /// This is intended to be a debugging tool; Graph should never intentionally
    /// hold malformed or cyclic graphs.
    fn validate(&self) -> Result<()> {
        // TODO
        Ok(())
    }
}

impl<T: PartialEq + Ord> PartialEq<Graph<T>> for Graph<T> {
    fn eq(&self, other: &Self) -> bool {
        self.edges == other.edges
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
    nodes: HashMap<VideoNodeId, VideoNode>,
    graph: Graph<VideoNodeId>,
    graph_dirty: RefCell<bool>,
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
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Serialize, Deserialize)]
struct State {
    #[serde(rename = "vertices")]
    node_ids: Vec<VideoNodeId>,
    edges: Vec<StateEdge>,
}

impl Model {
    pub fn new() -> Model {
        Model {
            graph: Graph::new(),
            nodes: Default::default(),
            graph_dirty: Default::default(),
        }
    }

    pub fn node(&self, id: VideoNodeId) -> Result<&VideoNode> {
        self.nodes
            .get(&id)
            .map(|n| n.borrow())
            .ok_or_else(|| Error::invalid_node_id(id))
    }

    pub fn node_mut(&mut self, id: VideoNodeId) -> Result<&mut VideoNode> {
        self.nodes
            .get_mut(&id)
            .map(|n| n.borrow_mut())
            .ok_or_else(|| Error::invalid_node_id(id))
    }

    pub fn nodes(&self) -> impl Iterator<Item = &VideoNode> {
        self.nodes.values().map(|n| n.borrow())
    }

    pub fn nodes_mut(&mut self) -> impl Iterator<Item = &mut VideoNode> {
        self.nodes.values_mut().map(|n| (*n).borrow_mut())
    }

    pub fn ids(&self) -> impl Iterator<Item = VideoNodeId> + '_ {
        self.nodes.keys().copied()
    }

    pub fn toposort(&self) -> Vec<&VideoNode> {
        self.graph
            .toposort()
            .unwrap()
            .iter()
            .map(|id| self.nodes.get(id).unwrap().borrow())
            .collect()
    }

    pub fn node_inputs(&self, node: &VideoNode) -> Vec<Option<&VideoNode>> {
        if let Ok(inputs) = self.graph.vertex_inputs(node.id()) {
            inputs.iter()
                .map(|input| {
                    input
                        .as_ref()
                        .map(|id| self.nodes.get(id))
                        .flatten()
                })
                .collect()
        } else {
            Vec::new()
        }
    }

    pub fn set_node_state(&mut self, id: VideoNodeId, state: JsonValue, library: &Library) -> Result<()> {
        if let Some(node) = self.nodes.get_mut(&id) {
            node.set_state(state)?;
        } else {
            let node = VideoNode::from_serde(state, library).unwrap();
            self.graph.set_vertex(id, Some(node.n_inputs()));
            self.nodes.insert(id, node);
            *self.graph_dirty.borrow_mut() = true;
        }

        Ok(())
    }

    pub fn add_node(&mut self, state: JsonValue, library: &Library) -> Result<VideoNodeId> {
        let node = VideoNode::from_serde(state, library)?;
        let id = node.id();
        self.graph.set_vertex(id, Some(node.n_inputs()));
        self.nodes.insert(id, node);

        *self.graph_dirty.borrow_mut() = true;

        Ok(id)
    }

    pub fn remove_node(&mut self, id: VideoNodeId) -> Result<()> {
        self.graph.set_vertex(id, None);
        self.nodes.remove(&id);
        *self.graph_dirty.borrow_mut() = true;
        Ok(())
    }

    pub fn clear(&mut self) {
        self.nodes.clear();
        self.graph.clear();
        *self.graph_dirty.borrow_mut() = true;
    }

    pub fn add_edge(&mut self, from: VideoNodeId, to: VideoNodeId, input: usize) -> Result<()> {
        self.graph.set_edge(Some(from), to, input)?;
        *self.graph_dirty.borrow_mut() = true;
        Ok(())
    }

    pub fn remove_edge(&mut self, _from: VideoNodeId, to: VideoNodeId, input: usize) -> Result<()> {
        self.graph.set_edge(None, to, input)?;
        *self.graph_dirty.borrow_mut() = true;
        Ok(())
    }

    pub fn state(&self) -> JsonValue {
        serde_json::to_value(&self.graph).unwrap()
    }

    pub fn set_state(&mut self, state_obj: JsonValue) -> Result<()> {
        let new_graph: Graph<VideoNodeId> = serde_json::from_value(state_obj)?;
        if new_graph != self.graph {
            if new_graph.toposort().is_ok() {
                self.graph = new_graph;
                *self.graph_dirty.borrow_mut() = true;
                info!("Graph replaced");
            } else {
                info!("Graph not replaced; has cycle!");
            }
        } else {
            info!("Graph not replaced; same");
        }
        Ok(())
    }

    pub fn flush(&self) -> bool {
        let dirty = RefCell::new(false);
        self.graph_dirty.swap(&dirty);
        dirty.into_inner()
    }

    /// Check nodes for any changes to n_inputs
    pub fn check_nodes(&mut self) {
        for (id, node) in self.nodes.iter() {
            self.graph.set_vertex(*id, Some(node.n_inputs()));
        }
    }
}
