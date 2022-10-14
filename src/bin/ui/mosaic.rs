use radiance::{Graph, NodeId};
use egui::Rect;
use std::collections::HashMap;

/// A unique identifier for a visual tile in the UI.
/// There may be multiple tiles per node,
/// since the graph may be a DAG but is visualized as a tree.
/// The "instance" field increments for each additional tile for the same NodeId.
#[derive(Debug, Eq, Hash, PartialEq, Clone, Copy)]
pub struct TileId {
    pub node: NodeId,
    pub instance: u32,
}

/// A struct describing a visual node in the graph.
/// There may be multiple visual nodes per graph node,
/// since the graph may be a DAG but is visualized as a tree.
/// The "instance" field tracks this.
#[derive(Debug, Clone)]
pub struct TilePlacement {
    pub id: TileId,     // The tile ID (includes node ID)
    pub rect: Rect,     // The visual rectangle to draw the node in
}

/// Visually lay out a graph
pub fn layout(graph: &Graph) -> Vec<TilePlacement> {
    // We will perform a DFS from each start node
    // to convert the graph (a DAG) into a tree.
    // Some nodes will be repeated.

    // Prepare to count repeated nodes
    // Initialize all node counts to zero
    let mut instance_count = HashMap::<NodeId, u32>::new();
    for &node_id in graph.iter_nodes() {
        instance_count.insert(node_id, 0);
    }

    let mut allocate_tile = |node: &NodeId| -> TileId {
        let count = instance_count.get_mut(node).unwrap();
        let orig_count = *count;
        *count += 1;
        TileId {
            node: *node,
            instance: orig_count,
        }
    };

    // Start by allocating tiles for the start nodes
    // (start nodes will necessarily have only one tile)
    let start_tiles: Vec<TileId> = graph.start_nodes().map(&mut allocate_tile).collect();

    let mut stack = start_tiles.clone();
    let mut tree = HashMap::<TileId, Vec<Option<TileId>>>::new();

    while let Some(tile) = stack.pop() {
        // For each node in the stack, 1) allocate new tiles for its children
        let child_tiles: Vec<Option<TileId>> = graph.input_mapping().get(&tile.node).unwrap()
            .iter()
            .map(|maybe_child_node| maybe_child_node.as_ref().map(&mut allocate_tile))
            .collect();

        // 2) Push its newly created child tiles onto the stack
        for &child_tile in child_tiles.iter().flatten() {
            stack.push(child_tile);
        }

        // 3) Insert its child connections into the tree
        tree.insert(tile, child_tiles);
    }

    // We now have a tree structure of tiles
    // (technically a forest)
    // represented by `start_tiles` (roots)
    // and `tree` (lookup table from tile -> child tiles)

    println!("start_tiles: {:?}", start_tiles);
    println!("tree: {:?}", tree);
}
