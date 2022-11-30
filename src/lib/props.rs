use crate::graph::{Graph, NodeId};
use crate::effect_node::EffectNodeProps;
use std::collections::{HashMap, HashSet};
use serde::{Serialize, Deserialize};

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
}

/// This is a struct of props that are common to every node.
pub struct CommonNodeProps {
    pub input_count: Option<u32>,
}

impl From<&NodeProps> for CommonNodeProps {
    fn from(props: &NodeProps) -> Self {
        match props {
            NodeProps::EffectNode(p) => p.into(),
        }
    }
}

/// `GlobalProps` govern the overall behavior not specific to any one node.
/// For example, `GlobalProps` has a `time` property representing the global timebase.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct GlobalProps {
    // TODO merge these fields into Props

    /// Time, in beats. Wraps around at 64 beats.
    pub time: f32,

    /// Time between successive calls to `update()`, in beats
    pub dt: f32,
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
    pub graph: Graph,
    pub node_props: HashMap<NodeId, NodeProps>,
    pub global_props: GlobalProps,
}

impl Props {

    // TODO delete a node by simply deleting it from the grpah.nodes
    // TODO add a node by pushing it onto graph.nodes (and node_props)
    // Then these methods can be removed
    // and less naive versions of them can be written into impl Graph

    /// Delete nodes
    pub fn delete_nodes(&mut self, delete_ids: &HashSet<NodeId>) {
        self.graph.nodes.retain(|id| !delete_ids.contains(id));
        self.graph.edges.retain(|edge| !delete_ids.contains(&edge.from) && !delete_ids.contains(&edge.to));
        self.node_props.retain(|id, _| !delete_ids.contains(id));
    }

    /// Add a node
    pub fn add_node(&mut self, id: NodeId, props: NodeProps) {
        self.graph.nodes.push(id);
        self.node_props.insert(id, props);
    }
}
