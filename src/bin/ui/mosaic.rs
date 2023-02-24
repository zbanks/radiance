#![allow(clippy::single_match)]

use radiance::{Props, Graph, NodeId, NodeProps, CommonNodeProps, NodeState, InsertionPoint};
use egui::{pos2, vec2, Rect, Ui, Widget, Response, InnerResponse, Vec2, Sense, Pos2, TextureId, Modifiers, IdMap, InputState};
use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Mutex};
use std::hash::Hash;
use crate::ui::tile::{Tile, TileId};
use crate::ui::drop_target::DropTarget;
use crate::ui::effect_node_tile::EffectNodeTile;
use crate::ui::screen_output_node_tile::ScreenOutputNodeTile;

const MARGIN: f32 = 20.;
const MOSAIC_ANIMATION_DURATION: f32 = 0.5;
const INTENSITY_SCROLL_RATE: f32 = 0.001;
const DROP_TARGET_WIDTH: f32 = 50.;
const DROP_TARGET_DISPLACEMENT: f32 = 10.;

/// A struct to hold info about a single tile that has been laid out.
#[derive(Clone, Debug)]
struct TileInMosaic {
    /// The visual tile
    pub tile: Tile,

    /// The insertion point that should be used if a tile is inserted on the output of this tile
    pub output_insertion_point: InsertionPoint,
}

/// A struct to hold info about a place in the mosaic where a subgraph may be inserted
#[derive(Clone, Debug)]
struct DropTargetInMosaic {
    /// The visual drop target
    pub drop_target: DropTarget,

    /// The insertion point that should be used if content is dropped onto this drop target
    pub insertion_point: InsertionPoint,
}

impl LayoutCache {
    fn from_props(props: &mut Props) -> Self {
        // TODO: take, as input, a map NodeId to TileSizeDescriptors
        // and use that instead of the `match` statement below

        let (start_nodes, input_mapping) = props.graph.mapping();

        // We will perform a DFS from each start node
        // to convert the graph (a DAG) into a tree.
        // Some nodes will be repeated.

        // Prepare to count repeated nodes
        // Initialize all node counts to zero
        let mut instance_count = HashMap::<NodeId, u32>::new();
        for &node_id in props.graph.nodes.iter() {
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

        // Start an map from a TileId to its output's insertion point.
        let mut output_insertion_points = HashMap::<TileId, InsertionPoint>::new();
        // Also start an map from a TileId to its inputs' insertion points.
        let mut input_insertion_points = HashMap::<TileId, Vec<InsertionPoint>>::new();

        // Start by allocating tiles for the start nodes
        // (start nodes will necessarily have only one tile)
        let start_tiles: Vec<TileId> = start_nodes.iter().map(&mut allocate_tile).collect();

        // Add a half-open output insertion point for each start tile
        for &tile_id in start_tiles.iter() {
            output_insertion_points.insert(tile_id, InsertionPoint {
                from_output: Some(tile_id.node),
                to_inputs: Default::default(),
            });
        }

        // Note that we frequently reverse the order things are pushed to the stack.
        // This prioritizes exploring start nodes that appear earlier in the nodes list,
        // as well as nodes connected to lower-numbered inputs,
        // causing them to be laid out first / at the top, consistently.
        let mut stack: Vec<TileId> = start_tiles.iter().rev().cloned().collect();
        let mut tree = HashMap::<TileId, Vec<Option<TileId>>>::new();

        while let Some(tile) = stack.pop() {
            // For each node in the stack, 1) allocate new tiles for its children
            let child_tiles: Vec<Option<TileId>> = input_mapping.get(&tile.node).unwrap()
                .iter()
                .map(|maybe_child_node| maybe_child_node.as_ref().map(&mut allocate_tile))
                .collect();

            // Look up the input count
            let props = props.node_props.get(&tile.node).unwrap();
            let input_count = CommonNodeProps::from(props).input_count.unwrap_or(1);

            // 2) Create output insertion points for each child,
            // and input insertion points for this node
            let mut my_input_insertion_points = Vec::<InsertionPoint>::new();
            for input in 0..input_count {
                let child_tile = child_tiles.get(input as usize).cloned().flatten();
                match child_tile {
                    Some(child_tile) => {
                        // Add a closed insertion point to both the inputs & outputs
                        // if the input is connected
                        let insertion_point = InsertionPoint {
                            from_output: Some(child_tile.node),
                            to_inputs: vec![(tile.node, input as u32)],
                        };
                        output_insertion_points.insert(child_tile, insertion_point.clone());
                        my_input_insertion_points.push(insertion_point);
                    },
                    None => {
                        // Add a half-open insertion point to just the inputs
                        // if the input is disconnected
                        let insertion_point = InsertionPoint {
                            from_output: None,
                            to_inputs: vec![(tile.node, input as u32)],
                        };
                        my_input_insertion_points.push(insertion_point);
                    }
                }
            }
            input_insertion_points.insert(tile, my_input_insertion_points);

            // 3) Push its newly created child tiles onto the stack
            for &child_tile in child_tiles.iter().rev().flatten() {
                stack.push(child_tile);
            }

            // 4) Insert its child connections into the tree
            tree.insert(tile, child_tiles);
        }

        // We now have a tree structure of tiles
        // (technically a forest)
        // represented by `start_tiles` (roots)
        // and `tree` (lookup table from tile -> child tiles)
        // and are ready to begin computing geometry.

        // We will start with the vertical geometry (heights and Y positions.)

        // To begin, lets make a mapping of TileId to its minimum input heights.
        let min_input_heights: HashMap<TileId, Vec<f32>> = tree.keys().map(|&tile_id| {
            let props = props.node_props.get(&tile_id.node).unwrap();
            let heights = match props {
                NodeProps::EffectNode(p) => EffectNodeTile::min_input_heights(p),
                NodeProps::ScreenOutputNode(p) => ScreenOutputNodeTile::min_input_heights(&p),
            };
            // The length of the returned heights array should match the input count,
            // or be 1 if the input count is zero
            assert_eq!(heights.len() as u32, 1.max(CommonNodeProps::from(props).input_count.unwrap_or(1)));

            (tile_id, heights)
        }).collect();

        // The actual input heights may be larger than this.
        // Initialize the actual input heights to the min heights plus some margin.
        let mut input_heights_full_size: HashMap<TileId, Vec<f32>> = min_input_heights.iter().map(|(&tile_id, heights)| {
            // Each input port will split its margin half above and half below.
            (tile_id, heights.iter().map(|h| h + MARGIN).collect())
        }).collect();

        // Also, lets make a mapping of Y coordinates.
        // We will initialize them to NAN to make sure we don't miss anything.
        let mut ys_full_size: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
            (tile_id, f32::NAN)
        }).collect();

        // Now traverse each tree, computing heights and Y positions in three passes
        let mut y = 0_f32;
        for &start_tile in start_tiles.iter() {
            set_input_height_fwd(&start_tile, &tree, &mut input_heights_full_size);
            set_input_height_rev(&start_tile, &tree, &mut input_heights_full_size);
            set_vertical_stackup(&start_tile, &tree, &input_heights_full_size, &mut ys_full_size, &mut y);
        }
        let total_height = y;

        // In a fourth pass, shrink nodes that are unnecessarily tall.
        // Just because a node is connected to a tall output
        // and is allocated a large amount of vertical space
        // doesn't mean it has to take it all.

        // We shrink the top-most and bottom-most input.
        // We can't mess with the inner inputs without messing up the layout.
        // This operation also won't affect the total height,
        // since that height was allocated for a reason.

        let mut inputs = HashMap::<TileId, Vec<f32>>::new();
        let mut outputs = HashMap::<TileId, f32>::new();
        let mut heights = HashMap::<TileId, f32>::new();
        let mut ys = HashMap::<TileId, f32>::new();

        for &tile_id in tree.keys() {
            let my_input_heights = input_heights_full_size.get_mut(&tile_id).unwrap();
            let my_min_input_heights = min_input_heights.get(&tile_id).unwrap();

            // See how much we can shrink the top input
            let shrinkage_top = 0.5 * (my_input_heights.first().unwrap() - my_min_input_heights.first().unwrap());
            assert!(shrinkage_top >= 0.);

            // See how much we can shrink the bottom input
            let shrinkage_bottom = 0.5 * (my_input_heights.last().unwrap() - my_min_input_heights.last().unwrap());
            assert!(shrinkage_bottom >= 0.);

            // Convert input heights to chevron locations (midpoint of each input port)
            let mut last_input_bottom_y = 0.;
            let my_inputs: Vec<f32> = my_input_heights.iter().map(|&input_height| {
                let input = last_input_bottom_y + 0.5 * input_height - shrinkage_top;
                last_input_bottom_y += input_height;
                input
            }).collect();
            // Remember this tile's total allocated height
            let allocated_height = last_input_bottom_y;

            // Compute and write the input chevron locations
            inputs.insert(tile_id, my_inputs);

            // Compute and write the output chevron location
            outputs.insert(tile_id, 0.5 * allocated_height - shrinkage_top);

            // Compute and write the new Y coordinate
            let &y = ys_full_size.get(&tile_id).unwrap();
            ys.insert(tile_id, y + shrinkage_top);

            // Compute and insert new height
            heights.insert(tile_id, allocated_height - shrinkage_top - shrinkage_bottom);
        }

        // We are done with the vertical geometry! Now for the horizontal geometry.

        // Tile widths are not flexible in the same way the heights are--
        // each node sets its own width.
        // However, it has the option to make its width dependent on its computed height
        // (that is why we do heights first.)
        let widths: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
            let height: f32 = *heights.get(&tile_id).unwrap();
            let node_props = props.node_props.get(&tile_id.node).unwrap();
            let width = match node_props {
                NodeProps::EffectNode(p) => EffectNodeTile::width_for_height(p, height),
                NodeProps::ScreenOutputNode(p) => ScreenOutputNodeTile::width_for_height(p, height),
            };
            (tile_id, width)
        }).collect();

        // We will initialize these to NAN like the ys
        let mut xs: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
            (tile_id, f32::NAN)
        }).collect();

        // X is simpler than Y, we only need one pass
        for &start_tile in start_tiles.iter() {
            set_horizontal_stackup(&start_tile, &tree, &widths, &mut xs, 0.);
        }

        // Well, three, if you count finding the total width and flipping the coordinate system.
        // Note that for the horizontal layout, we don't add any margin per-tile like we do for the vertical layout.
        // We only add 0.5 * MARGIN on the left and right of the entire mosaic.
        let total_width = *xs.values().max_by(|x, y| x.partial_cmp(y).unwrap()).unwrap_or(&0.) + MARGIN;
        for x in xs.values_mut() {
            *x = total_width - *x - MARGIN * 0.5;
        }

        // Collect and return the resulting tiles
        let tiles: Vec<TileInMosaic> = tree.keys().map(|&tile_id| {
            let &x = xs.get(&tile_id).unwrap();
            let &y = ys.get(&tile_id).unwrap();
            let &width = widths.get(&tile_id).unwrap();
            let &height = heights.get(&tile_id).unwrap();
            let my_inputs = inputs.get(&tile_id).unwrap().clone(); // Note: can probably .take instead of .clone
            let &my_output = outputs.get(&tile_id).unwrap();

            // TODO make outputs a f32 instead of Vec<f32> in Tile
            let my_outputs: Vec<f32> = [my_output].into();

            TileInMosaic {
                tile: Tile::new(
                    tile_id,
                    Rect::from_min_size(pos2(x, y), vec2(width, height)),
                    my_inputs,
                    my_outputs,
                ),
                output_insertion_point: output_insertion_points.get(&tile_id).unwrap().clone(),
            }
        }).collect();

        // Add drop targets for every input on every tile
        let mut drop_targets = Vec::<DropTargetInMosaic>::new();
        for &tile_id in tree.keys() {
            let my_input_heights = input_heights_full_size.get(&tile_id).unwrap();
            let &tile_x = xs.get(&tile_id).unwrap();
            let &tile_y = ys_full_size.get(&tile_id).unwrap();
            let my_input_insertion_points = input_insertion_points.get(&tile_id).unwrap();
            let x = tile_x - 0.5 * DROP_TARGET_WIDTH + DROP_TARGET_DISPLACEMENT;
            let mut y = tile_y;
            for (&input_height, insertion_point) in my_input_heights.iter().zip(my_input_insertion_points) {
                drop_targets.push(DropTargetInMosaic {
                    drop_target: DropTarget::new(
                        Rect::from_min_size(pos2(x, y), vec2(DROP_TARGET_WIDTH, input_height)),
                        true, // XXX temporary for debugging, don't make active unless hovered
                    ),
                    insertion_point: insertion_point.clone(),
                });
                y += input_height;
            }
        }

        // Add drop targets for every output on every start tile
        for &tile_id in start_tiles.iter() {
            let height_full_size = input_heights_full_size.get(&tile_id).unwrap().iter().sum();
            let width = widths.get(&tile_id).unwrap();
            let insertion_point = output_insertion_points.get(&tile_id).unwrap();
            let &tile_x = xs.get(&tile_id).unwrap();
            let &tile_y = ys_full_size.get(&tile_id).unwrap();
            let x = tile_x + width - 0.5 * DROP_TARGET_WIDTH + DROP_TARGET_DISPLACEMENT;
            let y = tile_y;

            drop_targets.push(DropTargetInMosaic {
                drop_target: DropTarget::new(
                    Rect::from_min_size(pos2(x, y), vec2(DROP_TARGET_WIDTH, height_full_size)),
                    true, // XXX temporary for debugging, don't make active unless hovered
                ),
                insertion_point: insertion_point.clone(),
            });
        }

        Self {
            graph: props.graph.clone(),
            size: vec2(total_width, total_height),
            tiles,
            drop_targets,
            start_tiles,
            tree
        }
    }
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

    for (i, &my_input_height) in input_heights.get(tile).unwrap().iter().enumerate() {
        let mut sub_y = *y;
        if let Some(child_tile) = child_tiles.get(i).cloned().flatten() {
            // For each child tile, place it farther down
            // according to the current tile's input heights
            set_vertical_stackup(&child_tile, tree, input_heights, ys, &mut sub_y);
        }
        *y += my_input_height;
    }
}

/// Working from roots to leaves, set the X position of each tile
/// Note that we work in a coordinate system where X increases from right to left
/// and we will flip it afterwards (once we know the total width)
fn set_horizontal_stackup(tile: &TileId, tree: &HashMap<TileId, Vec<Option<TileId>>>, widths: &HashMap<TileId, f32>, xs: &mut HashMap<TileId, f32>, x: f32) {
    let child_tiles = tree.get(tile).unwrap();

    // Place the current tile at the current X plus the tile width
    let sub_x = x + widths.get(tile).unwrap();
    *xs.get_mut(tile).unwrap() = sub_x;

    for &child_tile in child_tiles.iter().flatten() {
        set_horizontal_stackup(&child_tile, tree, widths, xs, sub_x);
    }
}

/// Given a set of start tiles (tiles with no connected outputs)
/// and a mapping from a given tile to its input tiles,
/// produce the inverse mapping:
/// one from a given tile to its output tile.
fn input_tree_to_output_tree(start_tiles: &[TileId], input_tree: &HashMap<TileId, Vec<Option<TileId>>>) -> HashMap<TileId, Option<TileId>> {
    let mut stack: Vec<TileId> = start_tiles.to_owned();
    let mut output_tree: HashMap<TileId, Option<TileId>> = start_tiles.iter().map(|&tile_id| (tile_id, None)).collect();

    while let Some(tile) = stack.pop() {
        // For each tile in the stack,
        for &child_tile in input_tree.get(&tile).unwrap().iter().flatten() {
            // 1) add an entry for each of its children
            output_tree.insert(child_tile, Some(tile));
            // 2) push each child onto the stack
            stack.push(child_tile);
        }
    }

    output_tree
}

/// Return a set of tiles that are both selected and connected (via other selected tiles) to the given target tile
fn selected_connected_component(target: &TileId, selected: &HashSet<NodeId>, start_tiles: &[TileId], input_tree: &HashMap<TileId, Vec<Option<TileId>>>) -> HashSet<TileId> {
    if !selected.contains(&target.node) {
        println!("Warning--dragging empty set");
        return HashSet::<TileId>::new();
    }

    let output_tree = input_tree_to_output_tree(start_tiles, input_tree);

    // 1) Walk downstream to find the "start" tile
    // of the selected connected component
    let mut start_tile = *target;
    while let Some(downstream_tile) = output_tree.get(&start_tile).unwrap() {
        if !selected.contains(&downstream_tile.node) {
            // Stop walking if we exited the selection
            break;
        }
        // Otherwise, keep walking
        start_tile = *downstream_tile;
    }

    // 2) From there, recursively search upstream
    // to get the whole subtree
    let mut result = HashSet::<TileId>::new();
    fn search_upstream(tile: &TileId, selected: &HashSet<NodeId>, tree: &HashMap<TileId, Vec<Option<TileId>>>, result: &mut HashSet<TileId>) {
        result.insert(*tile);
        for &child_tile in tree.get(tile).unwrap().iter().flatten() {
            if selected.contains(&child_tile.node) {
                search_upstream(&child_tile, selected, tree, result);
            }
        }
    }
    search_upstream(&start_tile, selected, input_tree, &mut result);
    result
}

// We implement a custom AnimationManager to get easing functions
#[derive(Debug, Default)]
struct MosaicAnimationManager {
    tiles: IdMap<TileAnimation>,
}

#[derive(Clone, Debug)]
struct TileAnimation {
    from_rect: Rect,
    to_rect: Rect,
    toggle_time_rect: f64,
    from_offset: Vec2,
    to_offset: Vec2,
    toggle_time_offset: f64,
    from_z: f32,
    to_z: f32,
    toggle_time_z: f64,
    from_alpha: f32, // opacity, not to be confused with easing function parameter
    to_alpha: f32,
    toggle_time_alpha: f64,
}

fn ease(x: f32) -> f32 {
    // Implement quadratic in-out easing

    let x = x.min(1.).max(0.);
    if x < 0.5 { 2. * x.powi(2) } else { 1. - 0.5 * (-2. * x + 2.).powi(2) }
}

fn rect_easing(time_since_toggle: f32, from_rect: Rect, to_rect: Rect) -> Rect {
    let alpha = ease(time_since_toggle / MOSAIC_ANIMATION_DURATION);

    let min = from_rect.min + (to_rect.min - from_rect.min) * alpha;
    let max = from_rect.max + (to_rect.max - from_rect.max) * alpha;
    Rect {
        min,
        max,
    }
}

fn vec_easing(time_since_toggle: f32, from_vec: Vec2, to_vec: Vec2) -> Vec2 {
    let alpha = ease(time_since_toggle / MOSAIC_ANIMATION_DURATION);

    from_vec + (to_vec - from_vec) * alpha
}

fn scalar_easing(time_since_toggle: f32, from_scalar: f32, to_scalar: f32) -> f32 {
    let alpha = ease(time_since_toggle / MOSAIC_ANIMATION_DURATION);

    from_scalar + (to_scalar - from_scalar) * alpha
}

impl MosaicAnimationManager {
    pub fn animate_tile(
        &mut self,
        input: &InputState,
        tile: Tile,
        dragging: bool,
    ) -> Tile {
        match self.tiles.get_mut(&tile.ui_id()) {
            None => {
                self.tiles.insert(
                    tile.ui_id(),
                    TileAnimation {
                        from_rect: tile.rect(),
                        to_rect: tile.rect(),
                        toggle_time_rect: -f64::INFINITY, // long time ago
                        from_offset: tile.offset(),
                        to_offset: tile.offset(),
                        toggle_time_offset: -f64::INFINITY,
                        from_z: tile.z(),
                        to_z: tile.z(),
                        toggle_time_z: -f64::INFINITY,
                        from_alpha: tile.alpha(),
                        to_alpha: tile.alpha(),
                        toggle_time_alpha: -f64::INFINITY,
                    },
                );
                tile
            }
            Some(anim) => {
                // Disable repeatedly triggering the offset animation while dragging
                // by always setting the current value to the target
                if dragging {
                    anim.to_offset = tile.offset();
                    // Also disable Z animation on drag start
                    // so the tiles are "lifted" immediately
                    anim.to_z = tile.z();
                }

                let time_since_toggle_rect = (input.time - anim.toggle_time_rect) as f32 + input.predicted_dt;
                let current_rect = rect_easing(
                    time_since_toggle_rect,
                    anim.from_rect,
                    anim.to_rect,
                );
                let time_since_toggle_offset = (input.time - anim.toggle_time_offset) as f32 + input.predicted_dt;
                let current_offset = vec_easing(
                    time_since_toggle_offset,
                    anim.from_offset,
                    anim.to_offset,
                );
                let time_since_toggle_z = (input.time - anim.toggle_time_z) as f32 + input.predicted_dt;
                let current_z = scalar_easing(
                    time_since_toggle_z,
                    anim.from_z,
                    anim.to_z,
                );
                let time_since_toggle_alpha = (input.time - anim.toggle_time_alpha) as f32 + input.predicted_dt;
                let current_alpha = scalar_easing(
                    time_since_toggle_alpha,
                    anim.from_alpha,
                    anim.to_alpha,
                );

                if anim.to_rect != tile.rect() {
                    anim.from_rect = current_rect;
                    anim.to_rect = tile.rect();
                    anim.toggle_time_rect = input.time;
                }
                if anim.to_offset != tile.offset() {
                    anim.from_offset = current_offset;
                    anim.to_offset = tile.offset();
                    anim.toggle_time_offset = input.time;
                }
                if anim.to_z != tile.z() {
                    anim.from_z = current_z;
                    anim.to_z = tile.z();
                    anim.toggle_time_z = input.time;
                }
                if anim.to_alpha != tile.alpha() {
                    anim.from_alpha = current_alpha;
                    anim.to_alpha = tile.alpha();
                    anim.toggle_time_alpha = input.time;
                }

                tile
                    .with_rect(current_rect)
                    .with_offset(current_offset)
                    .with_z(current_z)
                    .with_alpha(current_alpha)
            }
        }
    }

    pub fn retain_tiles(&mut self, tile_ids: &HashSet<egui::Id>) {
        self.tiles.retain(|id, _| tile_ids.contains(id));
    }
}

/// The last recorded graph, and the layout it produced
/// (stored to avoid re-computing layout if graph hasn't changed)
#[derive(Debug)]
struct LayoutCache {
    graph: Graph, // Stored to check equality against subsequent call
    size: Vec2,
    tiles: Vec<TileInMosaic>,
    drop_targets: Vec<DropTargetInMosaic>,
    start_tiles: Vec<TileId>,
    tree: HashMap<TileId, Vec<Option<TileId>>>,
}

/// State associated with dragged tiles
#[derive(Debug)]
struct DragMemory {
    pub target: TileId,
    pub contingent: HashSet<TileId>,
    pub offset: Vec2,
}

/// State associated with the mosaic UI, to be preserved between frames,
/// like which tiles are selected.
/// Does not include anything associated with the graph (like intensities)
#[derive(Debug, Default)]
struct MosaicMemory {
    pub animation_manager: MosaicAnimationManager,
    pub selected: HashSet<NodeId>,
    pub focused: Option<TileId>,
    pub drag: Option<DragMemory>,

    layout_cache: Option<LayoutCache>,
}

pub fn mosaic_ui<IdSource>(
    ui: &mut Ui,
    id_source: IdSource,
    props: &mut Props,
    node_states: &HashMap<NodeId, NodeState>,
    preview_images: &HashMap<NodeId, TextureId>,
    insertion_point: &mut InsertionPoint,
) -> Response
    where IdSource: Hash + std::fmt::Debug,
{
    // Generate an UI ID for the mosiac
    let mosaic_id = ui.make_persistent_id(id_source);

    // Load state from memory

    let mosaic_memory = ui.ctx().memory().data.get_temp_mut_or_default::<Arc<Mutex<MosaicMemory>>>(mosaic_id).clone();

    let mut mosaic_memory = mosaic_memory.lock().unwrap();

    props.fix();

    // Lay out the mosaic
    let layout_cache_needs_refresh = match &mosaic_memory.layout_cache {
        None => true,
        Some(cache) => cache.graph != props.graph,
    };

    if layout_cache_needs_refresh {
        props.graph.fix();
        mosaic_memory.layout_cache = Some(LayoutCache::from_props(props));

        // Retain only selected / focused / dragged nodes that still exist in the graph
        let graph_nodes: HashSet<NodeId> = props.graph.nodes.iter().cloned().collect();
        mosaic_memory.selected.retain(|id| graph_nodes.contains(id));
        if let Some(focused) = mosaic_memory.focused {
            if !graph_nodes.contains(&focused.node) {
                mosaic_memory.focused = None;
            }
        }
        let abort_drag = if let Some(drag) = &mosaic_memory.drag {
            if !graph_nodes.contains(&drag.target.node) {
                true // abort drag because primary target tile disappeared
            } else {
                // Recompute contingent based on
                // connected component from the drag target
                let new_contingent = selected_connected_component(
                    &drag.target,
                    &mosaic_memory.selected,
                    &mosaic_memory.layout_cache.as_ref().unwrap().start_tiles,
                    &mosaic_memory.layout_cache.as_ref().unwrap().tree
                );
                mosaic_memory.drag.as_mut().unwrap().contingent = new_contingent;
                false // don't abort drag
            }
        } else {
            false // no drag in progress
        };
        if abort_drag {
            mosaic_memory.drag = None;
        }
    }

    // Get layout from cache (guaranteed to exist post-refresh)
    let LayoutCache {size: layout_size, tiles, drop_targets, ..} = &mosaic_memory.layout_cache.as_ref().unwrap();
    let layout_size = *layout_size;
    let tiles = tiles.to_vec();
    let drop_targets = drop_targets.to_vec();

    let mosaic_rect = ui.available_rect_before_wrap();
    let mosaic_response = ui.allocate_rect(mosaic_rect, Sense::click());
    let scrollarea_offset = (mosaic_rect.min - Pos2::ZERO) + 0.5 * (mosaic_rect.size() - layout_size);

    // Apply focus, selection, drag, and animation

    // Translate each tile according to the scrollarea offset
    let tiles: Vec<TileInMosaic> = tiles.into_iter().map(|TileInMosaic {tile, output_insertion_point}| {
        let rect = tile.rect();
        let rect = rect.translate(scrollarea_offset);
        TileInMosaic {
            tile: tile.with_rect(rect),
            output_insertion_point,
        }
    }).collect();

    // Find the position of the upper left corner of the tile
    // that was the target of the drag
    // so we can reference the whole contingent from this point
    let drag_target_position = mosaic_memory.drag.as_ref()
        .map(|drag|
            tiles.iter().find(|&tile_in_mosaic| tile_in_mosaic.tile.id() == drag.target).unwrap()
            .tile.rect().min
        );

    insertion_point.clone_from(&Default::default());
    let mut tiles: Vec<Tile> = tiles.into_iter().map(|TileInMosaic {tile, output_insertion_point}| {
        let focused = match mosaic_memory.focused {
            Some(id) => id == tile.id(),
            None => false,
        };

        let selected = mosaic_memory.selected.contains(&tile.id().node);

        let (dragging, drag_offset) = mosaic_memory.drag.as_ref().and_then(|drag|
            drag.contingent.contains(&tile.id())
            .then(|| (true, drag.offset - (drag_target_position.unwrap() - Pos2::ZERO)))
        ).unwrap_or((false, Vec2::ZERO));

        if focused {
            // Use the focused tile's insertion point
            // as the overall graph's focused_insertion_point
            // TODO consider only updating this when the graph or the focus changes
            // TODO this is weird, returning a result in the parameters feels very C-like.
            // TODO tile focus is weird and probably needs to be done in-house
            // so it isn't cleared upon moving a slider or adding a node.
            // Right now it's impossible to clear this insertion point.
            insertion_point.clone_from(&output_insertion_point);
        }
        let tile = tile
            .with_focus(focused)
            .with_selected(selected)
            .with_offset(drag_offset)
            .with_lifted(dragging)
            .with_default_alpha()
            .with_default_z();
        let tile = mosaic_memory.animation_manager.animate_tile(&ui.input(), tile, dragging);
        tile
    }).collect();

    let drop_targets: Vec<DropTargetInMosaic> = drop_targets.into_iter().map(|DropTargetInMosaic {drop_target, insertion_point}| {
        let rect = drop_target.rect();
        let rect = rect.translate(scrollarea_offset);
        let drop_target = drop_target.with_rect(rect);
        DropTargetInMosaic {
            drop_target,
            insertion_point,
        }
    }).collect();

    // Sort
    tiles.sort_by_key(|tile| tile.draw_order());

    // Retain only animation states for tiles that exist
    let tile_ids = tiles.iter().map(|tile| tile.ui_id()).collect();
    mosaic_memory.animation_manager.retain_tiles(&tile_ids);

    // Draw

    // Set this variable when iterating over tiles to describe the drag situation
    enum DragSituation {
        None,
        Started(TileId, Vec2),
        Delta(Vec2),
        Released,
    }

    let mut drag_situation = DragSituation::None;

    for tile in tiles.into_iter() {
        let tile_id = tile.id();
        let tile_rect = tile.rect();
        let node_id = tile_id.node;
        let node_state = node_states.get(&node_id).unwrap();
        let &preview_image = preview_images.get(&node_id).unwrap();
        let node_props = props.node_props.get_mut(&node_id).unwrap();

        let InnerResponse { inner: _, response } = tile.show(ui, |ui| {
            match node_props {
                NodeProps::EffectNode(p) => EffectNodeTile::new(p, node_state.try_into().unwrap(), preview_image).add_contents(ui),
                NodeProps::ScreenOutputNode(p) => ScreenOutputNodeTile::new(p, node_state.try_into().unwrap(), preview_image).add_contents(ui),
            }
        });

        if response.dragged() || response.clicked() {
            // Ensure the mosaic stays focused throughout interaction
            // so it can pick up keyboard input
            mosaic_response.request_focus();
        }

        if response.drag_released() && mosaic_memory.drag.is_some() {
            drag_situation = DragSituation::Released;
        }

        // How we need to change selection based on interaction
        enum SelectionAction {
            None, // Do not select this tile
            Clicked, // Act as if this tile was clicked (e.g. deselect it if Ctrl is held)
            Dragged, // Act as if this file was clicked, but ensure that it ends up selected
        }

        let mut selection_action = SelectionAction::None;

        if response.dragged() {
            let delta = response.drag_delta();
            match &mosaic_memory.drag {
                Some(_) => {
                    // We have an existing drag. Apply our delta.
                    drag_situation = DragSituation::Delta(delta);
                },
                None => {
                    // See if we have moved a nonzero amount. If so, begin the drag.
                    if delta != Vec2::ZERO {
                        // Workaround bug in egui: Discard the first delta,
                        // since it can be inaccurate when mixing touch + mouse
                        let offset = tile_rect.min - Pos2::ZERO;
                        drag_situation = DragSituation::Started(tile_id, offset);
                        // Treat starting a drag like a click,
                        // but ensure the tile is selected
                        // (so we never drag a deselected tile)
                        selection_action = SelectionAction::Dragged;
                    }
                },
            }
        } else if response.clicked() && matches!(drag_situation, DragSituation::None) {
            selection_action = SelectionAction::Clicked;
        }

        match selection_action {
            // TODO this match is sort of messy, with a lot of internal logic
            // differentiating between Clicked and Dragged
            SelectionAction::Clicked | SelectionAction::Dragged => {
                // Focus the tile
                mosaic_memory.focused = Some(tile_id);

                match ui.input().modifiers {
                    Modifiers { ctrl: true, .. } => {
                        if mosaic_memory.selected.contains(&node_id) {
                            // Only allow removal from selection
                            // if we don't need to ensure selected
                            if !matches!(selection_action, SelectionAction::Dragged) {
                                mosaic_memory.selected.remove(&node_id);
                            }
                        } else {
                            mosaic_memory.selected.insert(node_id);
                        }
                    },
                    _ => {
                        // If we drag an already-selected tile, don't deselet everything else.
                        if !mosaic_memory.selected.contains(&node_id)
                           || !matches!(selection_action, SelectionAction::Dragged) {
                            // If we click, or if we drag on a tile that is not selected,
                            // then do normal behavior: deselect everything but the target tile.
                            mosaic_memory.selected.clear();
                            mosaic_memory.selected.insert(node_id);
                        }
                    },
                }
            },
            SelectionAction::None => {},
        }
    }

    // If we are dragging, render drop targets
    let mut hovered_insertion_point: Option<InsertionPoint> = None;
    if let Some(drag) = &mosaic_memory.drag {
        for drop_target in drop_targets.into_iter() {
            // Don't show drop targets that are attached to the drag contingent
            let mut attached_nodes = drop_target.insertion_point.from_output.iter().chain(
                drop_target.insertion_point.to_inputs.iter().map(|(n, _)| n)
            );
            let contingent_nodes: HashSet<NodeId> = drag.contingent.iter().map(|tile_id| tile_id.node).collect();
            if attached_nodes.any(|n| contingent_nodes.contains(n)) {
                continue;
            }

            let response = ui.add(drop_target.drop_target);
            if response.hovered() {
                hovered_insertion_point = Some(drop_target.insertion_point);
            }
        }
    }

    match drag_situation {
        DragSituation::Started(tile_id, offset) => {
            let LayoutCache {start_tiles, tree, ..} = &mosaic_memory.layout_cache.as_ref().unwrap();

            let contingent = selected_connected_component(&tile_id, &mosaic_memory.selected, start_tiles, tree);
            mosaic_memory.drag = Some(DragMemory {
                target: tile_id,
                contingent,
                offset,
            });
        },
        DragSituation::Delta(delta) => {
            let drag = mosaic_memory.drag.as_mut().unwrap(); // Don't emit this drag situation if None
            drag.offset += delta;
        },
        DragSituation::Released => {
            match hovered_insertion_point {
                Some(hovered_insertion_point) => {
                    // TODO: move_nodes is heavy-handed when applied to a non-tree-like structure.
                    // Consider figuring out exactly which edges to break (the ones incoming & outgoing to the *tiles* that are dragged)
                    // instead of breaking all incoming & outgoing edges to the *nodes* that are dragged
                    let nodes: HashSet<NodeId> = mosaic_memory.drag.as_ref().unwrap().contingent.iter().map(|tile_id| tile_id.node).collect();
                    props.graph.move_nodes(&nodes, &hovered_insertion_point);
                },
                None => {},
            }
            mosaic_memory.drag = None;
        },
        DragSituation::None => {},
    };

    // Check if background was clicked, and if so, blur, deselect, and drop tiles
    if mosaic_response.clicked() {
        // Focus the mosaic
        mosaic_response.request_focus();

        // No tile is focused
        mosaic_memory.focused = None;
        match ui.input().modifiers {
            Modifiers { ctrl: true, .. } => {
                // Do nothing if Ctrl-click
            },
            _ => {
                // Deselect all tiles for normal click
                mosaic_memory.selected.clear();
            },
        }

        // Drop tiles if they are lifted
        mosaic_memory.drag = None;
    }

    // Graph interactions
    if mosaic_response.has_focus() {
        // Handle scroll wheel
        let intensity_delta = ui.input().scroll_delta.y * INTENSITY_SCROLL_RATE;
        if intensity_delta != 0. {
            for node in mosaic_memory.selected.iter() {
                match props.node_props.get_mut(node).unwrap() {
                    NodeProps::EffectNode(node_props) => {
                        if let Some(intensity) = node_props.intensity {
                            node_props.intensity = Some((intensity + intensity_delta).clamp(0., 1.));
                        }
                    },
                    _ => {},
                }
            }
        }

        // Handle delete key
        if ui.input().key_pressed(egui::Key::Delete) {
            props.graph.delete_nodes(&mosaic_memory.selected);
        }

        // Handle R key (reload)
        if ui.input().key_pressed(egui::Key::R) {
            // Reload selected nodes
            mosaic_memory.selected = mosaic_memory.selected.iter().map(|&selected_node_id| {
                let new_node_id = NodeId::gen();
                let node_props = props.node_props.get(&selected_node_id).unwrap().clone();
                props.node_props.insert(new_node_id, node_props);
                props.node_props.remove(&selected_node_id);
                props.graph.replace_node(selected_node_id, new_node_id);
                // Make sure the node stays selected
                new_node_id
            }).collect();
        }
    }

    mosaic_response
}

pub fn mosaic<'a, IdSource>(
    id_source: IdSource,
    props: &'a mut Props,
    node_states: &'a HashMap<NodeId, NodeState>,
    preview_images: &'a HashMap<NodeId, TextureId>,
    insertion_point: &'a mut InsertionPoint,
) -> impl Widget + 'a
    where IdSource: Hash + std::fmt::Debug + 'a,
{
    move |ui: &mut Ui| mosaic_ui(
        ui,
        id_source,
        props,
        node_states,
        preview_images,
        insertion_point,
    )
}
