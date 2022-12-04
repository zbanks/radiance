use radiance::{Props, Graph, NodeId, NodeProps, CommonNodeProps, NodeState, InsertionPoint};
use egui::{pos2, vec2, Rect, Ui, Widget, Response, InnerResponse, Vec2, Sense, Pos2, TextureId, Modifiers, IdMap, InputState};
use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Mutex};
use std::hash::Hash;
use crate::ui::tile::{Tile, TileId};
use crate::ui::effect_node_tile::EffectNodeTile;

const MARGIN: f32 = 20.;
const MOSAIC_ANIMATION_DURATION: f32 = 0.5;

/// A struct to hold info about a single tile that has been laid out.
#[derive(Clone, Debug)]
struct TileInMosaic {
    /// The visual tile
    pub tile: Tile,

    /// The insertion point that should be used if a tile is inserted on the output of this tile
    pub output_insertion_point: InsertionPoint,
}

/// Visually lay out the mosaic (collection of tiles)
fn layout(props: &mut Props) -> (Vec2, Vec<TileInMosaic>) {
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

    let mut stack = start_tiles.clone();
    let mut tree = HashMap::<TileId, Vec<Option<TileId>>>::new();

    while let Some(tile) = stack.pop() {
        // For each node in the stack, 1) allocate new tiles for its children
        let child_tiles: Vec<Option<TileId>> = input_mapping.get(&tile.node).unwrap()
            .iter()
            .map(|maybe_child_node| maybe_child_node.as_ref().map(&mut allocate_tile))
            .collect();

        // 2) Create output insertion points for each child
        for (input, &child_tile) in child_tiles.iter().enumerate() {
            if let Some(child_tile) = child_tile {
                output_insertion_points.insert(child_tile, InsertionPoint {
                    from_output: Some(child_tile.node),
                    to_inputs: vec![(tile.node, input as u32)],
                });
            }
        }

        // 3) Push its newly created child tiles onto the stack
        for &child_tile in child_tiles.iter().flatten() {
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
            NodeProps::EffectNode(p) => EffectNodeTile::min_input_heights(&p),
        };
        // The length of the returned heights array should match the input count,
        // or be 1 if the input count is zero
        assert_eq!(heights.len() as u32, 1.max(CommonNodeProps::from(props).input_count.unwrap_or(1)));

        (tile_id, heights)
    }).collect();

    // The actual input heights may be larger than this.
    // Initialize the actual input heights to the min heights plus some margin.
    let mut input_heights: HashMap<TileId, Vec<f32>> = min_input_heights.iter().map(|(&tile_id, heights)| {
        // Each input port will split its margin half above and half below.
        (tile_id, heights.iter().map(|h| h + MARGIN).collect())
    }).collect();

    // Also, lets make a mapping of Y coordinates.
    // We will initialize them to NAN to make sure we don't miss anything.
    let mut ys: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
        (tile_id, f32::NAN)
    }).collect();

    // Now traverse each tree, computing heights and Y positions in three passes
    let mut y = 0_f32;
    for &start_tile in start_tiles.iter() {
        set_input_height_fwd(&start_tile, &tree, &mut input_heights);
        set_input_height_rev(&start_tile, &tree, &mut input_heights);
        set_vertical_stackup(&start_tile, &tree, &input_heights, &mut ys, &mut y);
    }
    let total_height = y;

    // In a fourth pass, shrink nodes that are unnecessarily tall.
    // Kust because a node is connected to a tall input
    // and is allocated a large amount of vertical space
    // doesn't mean it has to take it all.

    // We shrink the top-most and bottom-most input.
    // We can't mess with the inner inputs without messing up the layout.
    // This operation also won't affect the total height,
    // since that height was allocated for a reason.

    let mut inputs = HashMap::<TileId, Vec<f32>>::new();
    let mut outputs = HashMap::<TileId, f32>::new();
    let mut heights = HashMap::<TileId, f32>::new();

    for &tile_id in tree.keys() {
        let my_input_heights = input_heights.get(&tile_id).unwrap();
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
        let y = ys.get_mut(&tile_id).unwrap();
        *y = *y + shrinkage_top;

        // Compute and insert new height
        heights.insert(tile_id, allocated_height - shrinkage_top - shrinkage_bottom);
    }

    // We are done with the vertical geometry! Now for the horizontal geometry.

    // Tile widths are not flexible in the same way the heights are--
    // each node sets its own width.
    // However, it has the option to make its width dependent on its computed height
    // (that is why we do heights first.)
    let widths: HashMap<TileId, f32> = tree.keys().map(|&tile_id| {
        let height: f32 = input_heights.get(&tile_id).unwrap().iter().sum();
        let node_props = props.node_props.get(&tile_id.node).unwrap();
        let width = match node_props {
            NodeProps::EffectNode(p) => EffectNodeTile::width_for_height(&p, height),
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

    // Collect and return the result
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

    (vec2(total_width, total_height), tiles)
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

// We implement a custom AnimationManager to get easing functions
#[derive(Debug, Default)]
struct MosaicAnimationManager {
    tiles: IdMap<TileAnimation>,
}

#[derive(Clone, Debug)]
struct TileAnimation {
    from_rect: Rect,
    to_rect: Rect,
    /// when did `value` last toggle?
    toggle_time: f64,
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

impl MosaicAnimationManager {
    pub fn animate_tile(
        &mut self,
        input: &InputState,
        tile: Tile,
    ) -> Tile {
        match self.tiles.get_mut(&tile.ui_id()) {
            None => {
                self.tiles.insert(
                    tile.ui_id(),
                    TileAnimation {
                        from_rect: tile.rect(),
                        to_rect: tile.rect(),
                        toggle_time: -f64::INFINITY, // long time ago
                    },
                );
                tile
            }
            Some(anim) => {
                let time_since_toggle = (input.time - anim.toggle_time) as f32;
                // On the frame we toggle we don't want to return the old value,
                // so we extrapolate forwards:
                let time_since_toggle = time_since_toggle + input.predicted_dt;
                let current_rect = rect_easing(
                    time_since_toggle,
                    anim.from_rect,
                    anim.to_rect,
                );
                if anim.to_rect != tile.rect() {
                    anim.from_rect = current_rect; //start new animation from current position of playing animation
                    anim.to_rect = tile.rect();
                    anim.toggle_time = input.time;
                }
                tile.with_rect(current_rect)
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
}

/// State associated with the mosaic UI, to be preserved between frames,
/// like which tiles are selected.
/// Does not include anything associated with the graph (like intensities)
/// or anything already tracked separately by egui (like focus)
#[derive(Debug, Default)]
struct MosaicMemory {
    pub animation_manager: MosaicAnimationManager,
    pub selected: HashSet<NodeId>,

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
    props.fix();

    // Generate an UI ID for the mosiac
    let mosaic_id = ui.make_persistent_id(id_source);

    // Load state from memory

    let mosaic_memory = ui.ctx().memory().data.get_temp_mut_or_default::<Arc<Mutex<MosaicMemory>>>(mosaic_id).clone();

    let mut mosaic_memory = mosaic_memory.lock().unwrap();

    // Retain only selected nodes that still exist in the graph
    let graph_nodes: HashSet<NodeId> = props.graph.nodes.iter().cloned().collect();
    mosaic_memory.selected.retain(|id| graph_nodes.contains(id));

    // Lay out the mosaic
    let layout_cache_needs_refresh = match &mosaic_memory.layout_cache {
        None => true,
        Some(cache) => cache.graph != props.graph,
    };

    if layout_cache_needs_refresh {
        let (size, tiles) = layout(props);
        mosaic_memory.layout_cache = Some(LayoutCache {
            graph: props.graph.clone(),
            size,
            tiles,
        });
    }

    // Get layout from cache (guaranteed to exist post-refresh)
    let LayoutCache {graph: _, size: layout_size, tiles} = &mosaic_memory.layout_cache.as_ref().unwrap();
    let layout_size = *layout_size;
    let tiles = tiles.to_vec();

    let (mosaic_rect, mosaic_response) = ui.allocate_exact_size(layout_size, Sense {click: false, drag: false, focusable: false});

    // Note that the layout function returns tiles that go all the way to (0, 0)
    // and they must be further offset for the UI region we are allocated
    // Apply focus and offset,
    // and figure out where the focused insertion point is
    let offset = mosaic_rect.min - Pos2::ZERO;
    let mut any_tile_focused = false;
    let mut tiles: Vec<Tile> = tiles.into_iter().map(|TileInMosaic {tile, output_insertion_point}| {
        let focused = ui.ctx().memory().has_focus(tile.ui_id());
        let selected = mosaic_memory.selected.contains(&tile.id().node);
        any_tile_focused |= focused;
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
        let tile = mosaic_memory.animation_manager.animate_tile(&ui.input(), tile);
        tile.with_offset(offset).with_focus(focused).with_selected(selected)
    }).collect();

    // Sort
    tiles.sort_by_key(|tile| tile.draw_order());

    // Retain only animation states for tiles that exist
    let tile_ids = tiles.iter().map(|tile| tile.ui_id()).collect();
    mosaic_memory.animation_manager.retain_tiles(&tile_ids);

    // Draw
    for tile in tiles.into_iter() {
        let node_id = tile.id().node;
        let node_state = node_states.get(&node_id).unwrap();
        let &preview_image = preview_images.get(&node_id).unwrap();
        let node_props = props.node_props.get_mut(&node_id).unwrap();

        let InnerResponse { inner, response } = tile.with_offset(mosaic_rect.min - Pos2::ZERO).show(ui, |ui| {
            match node_props {
                NodeProps::EffectNode(p) => EffectNodeTile::new(p, node_state.try_into().unwrap(), preview_image).add_contents(ui),
            }
        });

        if response.clicked() {
            // Focus the tile
            response.request_focus();

            // Handle node selection
            match ui.input().modifiers {
                Modifiers { ctrl: true, .. } => {
                    if mosaic_memory.selected.contains(&node_id) {
                        mosaic_memory.selected.remove(&node_id);
                    } else {
                        mosaic_memory.selected.insert(node_id);
                    }
                },
                _ => {
                    mosaic_memory.selected.clear();
                    mosaic_memory.selected.insert(node_id);
                },
            }
        }
    }

    // Graph interactions
    if any_tile_focused && ui.input().key_pressed(egui::Key::Delete) {
        props.graph.delete_nodes(&mosaic_memory.selected);
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
