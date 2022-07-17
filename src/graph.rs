use crate::effect_node::EffectNodeProps;
use rand::Rng;
use std::collections::{HashMap, HashSet};
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

/// An edge in the graph is identified by a source `NodeId` ("from"),
/// a sink `NodeId` ("to"), and which input of the sink node to use ("input").
#[derive(Debug, Clone, Serialize, Deserialize, Eq, Hash, PartialEq)]
pub struct Edge {
    from: NodeId,
    to: NodeId,
    input: u32,
}

/// `GlobalProps` govern the overall behavior of the `Graph`.
/// For example, an `GlobalProps` has a properties of `time`.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
#[serde(tag = "type")]
pub struct GlobalProps {
    /// Time, in beats. Wraps around at 64 beats.
    pub time: f32,

    /// Time between successive calls to `update()`, in seconds
    pub dt: f32,
}

/// A `Graph` contains a list of nodes (such as effects, movies, and images)
/// and edges (which nodes feed into which other nodes)
/// that describe an overall visual composition.
/// 
/// Each node is identified by a `NodeId` and is stored in a sorted list.
/// The ordering of the list does not affect updating and painting,
/// but may be used for when visualizing the graph in the UI
///
/// Each node also has properties, accessed via `node_props()`,
/// describing that node's behavior.
/// 
/// The graph topology must be acyclic.
/// 
/// This `Graph` object is only the graph description:
/// It does not contain any render state or graphics resources.
/// One use case of a Graph is passing it to `Context.paint` for rendering.
/// Another is serializing it out to disk,
/// or deserializing it from a server.
#[derive(Debug, Clone, Serialize, Default)]
pub struct Graph {
    nodes: Vec<NodeId>,
    edges: Vec<Edge>,
    node_props: HashMap<NodeId, NodeProps>,
    global_props: GlobalProps,
}

impl<'de> Deserialize<'de> for Graph {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        #[derive(Deserialize)]
        struct UncheckedGraph {
            nodes: Vec<NodeId>,
            edges: Vec<Edge>,
            node_props: HashMap<NodeId, NodeProps>,
            global_props: GlobalProps,
        }

        impl TryFrom<UncheckedGraph> for Graph {
            type Error = &'static str;

            fn try_from(x: UncheckedGraph) -> Result<Self, Self::Error> {
                let set1: HashSet<NodeId> = x.nodes.iter().cloned().collect();
                let set2: HashSet<NodeId> = x.node_props.keys().cloned().collect();

                // Ensure there are no duplicates in the list of NodeIds
                if x.nodes.len() > set1.len() {
                    return Err("Node ID list has duplicates")
                }

                // Ensure the list of NodeIds matches the mapping of NodeIds to props
                if set1.symmetric_difference(&set2).next().is_some() {
                    return Err("Node ID list does not match node_props mapping")
                }

                Ok(Graph {
                    nodes: x.nodes,
                    edges: x.edges,
                    node_props: x.node_props,
                    global_props: x.global_props,
                })
            }
        }

        UncheckedGraph::deserialize(deserializer)?.try_into().map_err(D::Error::custom)
    }
}

// (TODO) after deserialization, the graph should be validated
// to ensure that every ID in `nodes` has a corresponding entry in `node_props`
// and that the graph topology is acyclic.

// (TODO) we should have a GraphOperation datum for easy implementation of undo / redo
// and sending changesets to a server.
// The server can then respond with the complete updated Graph

impl Graph {
    /// Iterate over the graph nodes, returning a reference to each NodeId
    pub fn iter_nodes(&self) -> impl Iterator<Item=&NodeId> {
        self.nodes.iter()
    }

    /// Get an individual node's properties
    pub fn node_props(&self, id: &NodeId) -> Option<&NodeProps> {
        self.node_props.get(id)
    }

    /// Get an individual node's properties for mutation
    pub fn node_props_mut(&mut self, id: &NodeId) -> Option<&mut NodeProps> {
        self.node_props.get_mut(id)
    }

    /// Get the global props for this graph
    pub fn global_props(&self) -> &GlobalProps {
        &self.global_props
    }

    /// Get the global props for this graph
    pub fn global_props_mut(&mut self) -> &mut GlobalProps {
        &mut self.global_props
    }
}
