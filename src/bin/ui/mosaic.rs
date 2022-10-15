use radiance::{Graph, NodeId, NodeProps, CommonNodeProps};
use egui::Rect;
use std::collections::HashMap;
use crate::ui::effect_node_tile::EffectNodeTile;

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
    // and are ready to begin computing geometry.

    println!("start_tiles: {:?}", start_tiles);
    println!("tree: {:?}", tree);

    // To begin, lets make a mapping of TileId to input height
    // and initialize them all to the min input heights.
    let mut input_heights: HashMap<TileId, Vec<f32>> = tree.keys().map(|&tile_id| {
        let props = graph.node_props(&tile_id.node).unwrap();
        let heights = match props {
            NodeProps::EffectNode(p) => EffectNodeTile::min_input_heights(p),
        };
        // The length of the returned heights array should match the input count,
        // or be 1 if the input count is zero
        assert_eq!(heights.len() as u32, 1.max(CommonNodeProps::from(props).input_count));
        (tile_id, heights)
    }).collect();

    // Now lets do widths.
    // We will initialize these to NAN to make sure we don't miss anything.
    let mut widths: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
        (tile_id, f32::NAN)
    }).collect();

    // Finally, lets make mappings for X and Y coordinates.
    // We will initialize these to NAN to make sure we don't miss anything.
    let mut xs: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
        (tile_id, f32::NAN)
    }).collect();

    let mut ys: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
        (tile_id, f32::NAN)
    }).collect();

    // Now traverse each tree, computing heights and Y positions
    let mut y = 0_f32;
    for &start_tile in start_tiles.iter() {
        set_input_height_fwd(&start_tile, &tree, &mut input_heights);
        set_input_height_rev(&start_tile, &tree, &mut input_heights);
        set_vertical_stackup(&start_tile, &tree, &input_heights, &mut ys, &mut y);
    }
    let total_height = y;

    println!("input heights: {:?}", input_heights);
    println!("input ys: {:?}", ys);
    println!("total height: {:?}", total_height);

    Vec::new() // XXX
}

/// Working from leaves to roots, set each input height to be large enough
/// to accomodate the total height of the tile attached to that input.
fn set_input_height_fwd(tile: &TileId, tree: &HashMap<TileId, Vec<Option<TileId>>>, input_heights: &mut HashMap<TileId, Vec<f32>>) {
    let input_count = input_heights.get(tile).unwrap().len();
    let child_tiles = tree.get(tile).unwrap();
    for i in 0..input_count {
        if let Some(child_tile) = child_tiles.get(i).cloned().flatten() {
            // Compute all upstream tile heights first
            // so we can use them to inform our height
            set_input_height_fwd(&child_tile, tree, input_heights);

            // Sum the child's input heights to get its overall height
            let child_total_height = input_heights.get(&child_tile).unwrap().iter().sum();
            // Expand our input height accordingly, if necessary
            let my_input_height = input_heights.get_mut(tile).unwrap().get_mut(i).unwrap();
            *my_input_height = my_input_height.max(child_total_height);
        }
    }
}

/// Working from roots to leaves, set the overall height of each child
/// to match the height of the input it is connected to
fn set_input_height_rev(tile: &TileId, tree: &HashMap<TileId, Vec<Option<TileId>>>, input_heights: &mut HashMap<TileId, Vec<f32>>) {
    let input_count = input_heights.get(tile).unwrap().len();
    let child_tiles = tree.get(tile).unwrap();
    for i in 0..input_count {
        if let Some(child_tile) = child_tiles.get(i).cloned().flatten() {
            let my_input_height = input_heights.get(tile).unwrap()[i];
            let child_total_height: f32 = input_heights.get(&child_tile).unwrap().iter().sum();
            if child_total_height < my_input_height {
                // Child is too small; uniformly expand all its inputs
                let correction_factor = my_input_height / child_total_height;
                for input_height in input_heights.get_mut(&child_tile).unwrap().iter_mut() {
                    *input_height *= correction_factor;
                }
            }
            // Recurse on downstream tiles
            set_input_height_rev(&child_tile, tree, input_heights);
        }
    }
}

/// Working from leaves to roots, set the Y position of each tile
fn set_vertical_stackup(tile: &TileId, tree: &HashMap<TileId, Vec<Option<TileId>>>, input_heights: &HashMap<TileId, Vec<f32>>, ys: &mut HashMap<TileId, f32>, y: &mut f32) {
    let child_tiles = tree.get(tile).unwrap();

    // Place the current tile at the current Y
    *ys.get_mut(tile).unwrap() = *y;

    let mut sub_y = *y;
    for (i, &my_input_height) in input_heights.get(tile).unwrap().iter().enumerate() {
        if let Some(child_tile) = child_tiles.get(i).cloned().flatten() {
            // For each child tile, place it farther down
            // according to the current tile's input heights
            set_vertical_stackup(&child_tile, tree, input_heights, ys, &mut sub_y);
        }
        *y += my_input_height;
    }
}
