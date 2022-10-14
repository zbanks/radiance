use radiance::{Graph, NodeId};
use egui::Rect;

/// A struct describing a visual node in the graph.
/// There may be multiple visual nodes per graph node,
/// since the graph may be a DAG but is visualized as a tree.
/// The "instance" field tracks this.
pub struct TilePlacement {
    pub id: NodeId,     // The node to be drawn here
    pub instance: u32,  // Which instance of the node (0 = first, 1 = second, ...)
    pub rect: Rect,     // The visual rectangle to draw the node in
}

/// Visually lay out a graph
pub fn layout(graph: &Graph) -> Vec<TilePlacement> {
    
}
