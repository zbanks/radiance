use crate::effect_node::EffectNodeProps;
use crate::graph::{Graph, NodeId};
use crate::image_node::ImageNodeProps;
pub use crate::mir::AudioLevels;
use crate::screen_output_node::ScreenOutputNodeProps;
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};

/// `NodeProps` govern the construction and behavior of a single node.
/// For example, an `EffectNode` has properties of `name` and `intensity`.
///
/// Some, but not all fields in `NodeProps` can be edited live.
/// For instance, editing an EffectNode's `intensity` every frame
/// is supported, but editing its `name` between successive paint calls
/// will likely cause the `EffectNode` to enter an error state.
/// To change an EffectNode's name, it must be re-added with a new ID.
///
/// NodeProps enumerates all possible node types,
/// and delegates to their specific props struct,
/// e.g. `EffectNodeProps`.
#[derive(Debug, Clone, Serialize, Deserialize, derive_more::TryInto)]
#[serde(tag = "type")]
#[try_into(owned, ref, ref_mut)]
pub enum NodeProps {
    EffectNode(EffectNodeProps),
    ScreenOutputNode(ScreenOutputNodeProps),
    ImageNode(ImageNodeProps),
}

/// This is a struct of props that are common to every node.
pub struct CommonNodeProps {
    pub input_count: Option<u32>,
}

impl From<&NodeProps> for CommonNodeProps {
    fn from(props: &NodeProps) -> Self {
        match props {
            NodeProps::EffectNode(p) => p.into(),
            NodeProps::ScreenOutputNode(p) => p.into(),
            NodeProps::ImageNode(p) => p.into(),
        }
    }
}

/// `Props` contains nodes (such as effects, movies, and images)
/// and connectivity information (which nodes feed into which other nodes)
/// that describe an overall visual composition.
///
/// Each node has properties, accessed via `node_props`,
/// describing that node's behavior.
///
/// This `Props` object is a descriptor:
/// It does not contain any render state or graphics resources.
/// One use case of a Props is passing it to `Context.paint` for rendering.
/// Another is serializing it out to disk,
/// or deserializing it from a server.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct Props {
    /// The list of node IDs and how they connect
    pub graph: Graph,

    /// Properties of each node, independent of connectivity
    pub node_props: HashMap<NodeId, NodeProps>,

    /// Time, in beats. Wraps around at 64 beats.
    #[serde(default)]
    pub time: f32,

    /// Time between successive calls to `update()`, in beats
    #[serde(default)]
    pub dt: f32,

    /// Audio levels
    #[serde(default)]
    pub audio: AudioLevels,
}

impl Props {
    /// Ensure that props is well-formed, and if it isn't, make it.
    /// Specifically, make sure that the list of nodes in node_props and graph match,
    /// and that there are no edges to non-existent nodes.
    pub fn fix(&mut self) {
        // a) remove any nodes from the graph that aren't present in node_props
        let graph_nodes_to_remove: HashSet<NodeId> = self
            .graph
            .nodes
            .iter()
            .filter(|n| !self.node_props.contains_key(n))
            .cloned()
            .collect();
        self.graph.delete_nodes(&graph_nodes_to_remove);
        let nodes_without_props = graph_nodes_to_remove.len();
        if nodes_without_props > 0 {
            println!("Removed {} nodes without props", nodes_without_props);
        }
        // b) remove any nodes from node_props that aren't present in the graph
        let save_nodes: HashSet<NodeId> = self.graph.nodes.iter().cloned().collect();
        let orig_nodes_len = self.graph.nodes.len();
        self.node_props.retain(|id, _| save_nodes.contains(id));
        let props_not_in_graph = orig_nodes_len - self.node_props.len();
        if props_not_in_graph > 0 {
            println!(
                "Removed {} node props that weren't in the graph",
                props_not_in_graph
            );
        }
        // c) remove any edges that go to a node input beyond a node's inputCount (if inputCount is Some)
        let orig_edges_len = self.graph.edges.len();
        self.graph.edges.retain(|edge| {
            save_nodes.contains(&edge.from)
                && save_nodes.contains(&edge.to)
                && match CommonNodeProps::from(self.node_props.get(&edge.to).unwrap()).input_count {
                    None => true, // Retain all edges if inputCount not yet known
                    Some(count) => edge.input < count,
                }
        });
        let edges_to_nonexistant_inputs = orig_edges_len - self.graph.edges.len();
        if edges_to_nonexistant_inputs > 0 {
            println!(
                "Removed {} edges to nonexistant inputs",
                edges_to_nonexistant_inputs
            );
        }
    }
}
