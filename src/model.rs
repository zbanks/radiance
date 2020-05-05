use crate::err::{Error, Result};
use crate::video_node::{IVideoNode, VideoNode, VideoNodeId};
use log::*;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::borrow::{Borrow, BorrowMut};
use std::cell::RefCell;
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};

// Graph is absracted over T to facilitate testing; but is practically for VideoNodeId
struct Graph<T: Copy + Ord + std::fmt::Debug> {
    /// Map of to_vertex -> [from_vertex_0, from_vertex_1, ...]
    edges: BTreeMap<T, Vec<Option<T>>>,
    toposort_cache: RefCell<Option<Vec<T>>>,
}

#[allow(dead_code, unused_variables)]
impl<T: Copy + Ord + std::fmt::Debug> Graph<T> {
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
        if let Some(fv) = from_vertex {
            if !self.edges.contains_key(&fv) {
                return Err(format!("from_vertex {:?} not in graph", fv).as_str().into());
            }
        }
        if !self.edges.contains_key(&to_vertex) {
            return Err(format!("to_vertex {:?} not in graph", to_vertex)
                .as_str()
                .into());
        }

        // TODO validation that this won't create a cycle

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
    dirt: RefCell<ModelDirt>,
}

#[derive(Default, Debug, Clone)]
pub struct ModelDirt {
    pub graph: bool,
    pub nodes: HashSet<VideoNodeId>,
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
            dirt: Default::default(),
        }
    }

    pub fn node(&self, id: VideoNodeId) -> Result<&VideoNode> {
        self.nodes
            .get(&id)
            .map(|n| n.borrow())
            .ok_or_else(|| Error::invalid_node_id(id))
    }

    pub fn node_mut(&mut self, id: VideoNodeId) -> Result<&mut VideoNode> {
        self.dirt.borrow_mut().nodes.insert(id);
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

    pub fn toposort(&self) -> Vec<&VideoNode> {
        self.graph
            .toposort()
            .unwrap()
            .iter()
            .map(|id| self.nodes.get(id).unwrap().borrow())
            .collect()
    }

    pub fn node_inputs(&self, node: &VideoNode) -> Vec<Option<&VideoNode>> {
        self.graph
            .vertex_inputs(node.id())
            .unwrap()
            .iter()
            .map(|input| {
                input
                    .as_ref()
                    .map(|id| self.nodes.get(id).unwrap().borrow())
            })
            .collect()
    }

    pub fn add_node(&mut self, state: JsonValue) -> Result<VideoNodeId> {
        let node = VideoNode::from_serde(state)?;
        let id = node.id();
        self.graph.set_vertex(id, Some(node.n_inputs()));
        self.nodes.insert(id, node);

        self.dirt.borrow_mut().graph = true;
        self.dirt.borrow_mut().nodes.insert(id);

        Ok(id)
    }

    pub fn remove_node(&mut self, id: VideoNodeId) -> Result<()> {
        self.graph.set_vertex(id, None);
        self.nodes.remove(&id);
        self.dirt.borrow_mut().graph = true;
        Ok(())
    }

    pub fn clear(&mut self) {
        self.nodes.clear();
        self.graph.clear();
        self.dirt.borrow_mut().graph = true;
    }

    pub fn add_edge(&mut self, from: VideoNodeId, to: VideoNodeId, input: usize) -> Result<()> {
        self.graph.set_edge(Some(from), to, input)?;
        self.dirt.borrow_mut().graph = true;
        Ok(())
    }

    pub fn remove_edge(&mut self, _from: VideoNodeId, to: VideoNodeId, input: usize) -> Result<()> {
        self.graph.set_edge(None, to, input)?;
        self.dirt.borrow_mut().graph = true;
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

        let edges = self
            .graph
            .all_edges()
            .map(|(a, b, i)| StateEdge {
                from_node: *id_map.get(&a).unwrap(),
                to_node: *id_map.get(&b).unwrap(),
                to_input: i,
            })
            .collect();
        let state = State { node_ids, edges };
        serde_json::to_value(&state).unwrap_or(JsonValue::Null)
    }

    pub fn flush(&self) -> ModelDirt {
        let dirt = RefCell::new(Default::default());
        self.dirt.swap(&dirt);
        dirt.into_inner()
    }
}
