use crate::effect_node::EffectNodeProps;
use rand::Rng;
use std::collections::{HashMap, HashSet, hash_map};
use serde::{Serialize, Deserialize};
use serde::de::Error;
use std::fmt;

/// A unique identifier that can be used to look up a `Node` in a `Graph`.
/// We use 128 bit IDs and assume that, as long as clients generate them randomly,
/// they will be unique and never collide, even across different application instances. 
#[derive(Eq, Hash, PartialEq, Debug, Clone, Copy)]
pub struct NodeId(u128);

impl NodeId {
    /// Generate a new random NodeId
    pub fn gen() -> NodeId {
        NodeId(rand::thread_rng().gen())
    }
}

impl fmt::Display for NodeId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "node_{}", &base64::encode(self.0.to_be_bytes())[0..22])
    }
}

impl Serialize for NodeId {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&self.to_string())
    }
}

impl<'de> Deserialize<'de> for NodeId {
    fn deserialize<D>(deserializer: D) -> Result<NodeId, D::Error>
        where D: serde::Deserializer<'de>
    {
        let s = String::deserialize(deserializer)?;
        if s.starts_with("node_") {
            let decoded_bytes: Vec<u8> = base64::decode(&s[5..]).map_err(D::Error::custom)?;
            let decoded_value = <u128>::from_be_bytes(decoded_bytes.try_into().map_err(|_| D::Error::custom("node id is wrong length"))?);
            Ok(NodeId(decoded_value))
        } else {
            Err(D::Error::custom("not a valid node id (must start with node_)"))
        }
    }
}

/// `NodeProps` govern the construction and behavior of a single node in a `Graph`.
/// For example, an `EffectNode` has properties of `name` and `intensity`.
/// 
/// Some, but not all fields in `NodeProps` can be edited live.
/// For instance, editing an EffectNode's `intensity` every frame
/// is supported, but editing its `name` between successive paint calls
/// will likely cause the `EffectNode` to enter an error state.
/// To change an EffectNode's name, it must be re-added to the `Graph`
/// with a new ID.
///
/// NodeProps enumerates all possible node types,
/// and delegates to their specific props struct,
/// e.g. `EffectNodeProps`.
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum NodeProps {
    EffectNode(EffectNodeProps),
}

/// A `Graph` contains a list of nodes (such as effects, movies, and images)
/// and edges (which nodes feed into which other nodes)
/// that describe an overall visual composition.
/// 
/// Each node is identified by a `NodeId` and contains a `NodeProps`
/// describing that node's behavior.
/// 
/// Each edge is identified by a source `NodeId` and a sink `NodeId` (TODO)
/// 
/// The graph topology must be acyclic.
/// 
/// This `Graph` object is only the graph description:
/// It does not contain any render state or graphics resources.
/// One use case of a Graph is passing it to `Context.paint` for rendering.
/// Another is serializing it out to disk,
/// or deserializing it from a server.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Graph {
    nodes: HashMap<NodeId, NodeProps>,
    edges: HashSet<(NodeId, NodeId)>,
}

// (TODO) we should have a GraphOperation datum for easy implementation of undo / redo
// and sending changesets to a server.
// The server can then respond with the complete updated Graph

impl Graph {
    /// Create an empty Graph
    pub fn new() -> Self {
        Self {
            nodes: HashMap::new(),
            edges: HashSet::new(),
        }
    }

    /// Add a node to the graph, with no edges.
    /// The given ID must be unique within the graph.
    pub fn insert_node(&mut self, id: NodeId, props: NodeProps) {
        assert!(!self.nodes.contains_key(&id), "Given node ID already exists in graph");
        self.nodes.insert(id, props);
    }

    /// Iterate over the graph nodes
    pub fn iter_nodes(&self) -> hash_map::Iter<NodeId, NodeProps> {
        self.nodes.iter()
    }

    /// Iterate over the graph nodes allowing mutation of the nodes
    pub fn iter_nodes_mut(&mut self) -> hash_map::IterMut<NodeId, NodeProps> {
        self.nodes.iter_mut()
    }
}
