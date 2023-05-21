use egui::{
    Ui, Widget, Sense, Rect, TextureId, Response, Id, Context, Layout, Align, Shape, Stroke, Color32, pos2, vec2, Vec2
};
use std::sync::{Arc, Mutex};
use std::collections::{HashMap};
use radiance::{NodeId, NodeState, Props, NodeProps};
use nalgebra::{Vector2, Vector3};

const SCRIM_FILL_COLOR: Color32 = Color32::BLACK;
const SCRIM_STROKE: Stroke = Stroke {
    width: 1.,
    color: Color32::GRAY,
};
const SCRIM_MARGIN: f32 = 30.;
const SCRIM_PADDING: f32 = 5.;

const EDITOR_OUTLINE_STROKE: Stroke = Stroke {
    width: 1.,
    color: Color32::GRAY,
};
const EDITOR_PT_SIZE: f32 = 10.;
const EDITOR_STROKE_SELECTED: Stroke = Stroke {
    width: 2.,
    color: Color32::WHITE,
};
const EDITOR_STROKE_BLURRED: Stroke = Stroke {
    width: 1.,
    color: Color32::GRAY,
};

/// State associated with the modal UI being shown, to be preserved between frames.
#[derive(Debug)]
pub enum ModalMemory {
    ProjectionMapEditor(NodeId), // TODO add node ID
}

pub fn modal_shown(ctx: &Context, id: Id) -> bool {
   ctx 
    .memory()
    .data
    .get_temp_mut_or_default::<Arc<Mutex<Option<ModalMemory>>>>(id)
    .clone()
    .lock()
    .unwrap()
    .is_some()
}

pub fn set_modal(ctx: &Context, id: Id, mm: Option<ModalMemory>) {
    ctx.memory().data.insert_temp(id, Arc::new(Mutex::new(mm)));
}

pub fn modal_ui(
    ui: &mut Ui,
    id: Id,
    props: &mut Props,
    node_states: &HashMap<NodeId, NodeState>,
    preview_images: &HashMap<NodeId, TextureId>,
) -> Response {

    // Load state from memory

    let modal_memory = ui
        .ctx()
        .memory()
        .data
        .get_temp_mut_or_default::<Arc<Mutex<Option<ModalMemory>>>>(id)
        .clone();

    let mut modal_memory = modal_memory.lock().unwrap();

    let available = ui.available_rect_before_wrap();
    let scrim_rect = available.shrink(SCRIM_MARGIN);
    let scrim = Shape::rect_filled(scrim_rect, 0., SCRIM_FILL_COLOR);
    ui.painter().add(scrim);
    let scrim_border = Shape::rect_stroke(scrim_rect, 0., SCRIM_STROKE);
    ui.painter().add(scrim_border);
    let ui_rect = scrim_rect.shrink(SCRIM_PADDING);

    let mut ui = ui.child_ui(ui_rect, Layout::top_down(Align::Center));
    match modal_memory.as_ref().expect("Tried to display modal but nothing currently shown") {
        ModalMemory::ProjectionMapEditor(node_id) => {
            if let Some(NodeProps::ProjectionMappedOutputNode(props)) = props.node_props.get(node_id) {
                let preview_image = preview_images.get(node_id).unwrap().clone();

                ui.heading("Projection Map Editor");
                // TODO: Add & remove screens
                // TODO: screen selector combobox
                ui.with_layout(
                    Layout::bottom_up(Align::Center).with_cross_justify(true),
                    |ui| {
                        if ui.button("Close").clicked() {
                            *modal_memory = None;
                        }
                        let available = ui.available_rect_before_wrap();
                        let midpoint = available.center().x;
                        let left = available.intersect(Rect::everything_left_of(midpoint - 0.5 * SCRIM_PADDING));
                        let right = available.intersect(Rect::everything_right_of(midpoint + 0.5 * SCRIM_PADDING));

                        let selected_screen_index = 0;

                        {
                            let mut ui = ui.child_ui(left, Layout::top_down(Align::Center).with_cross_justify(true));
                            ui.heading("Virtual:");

                            let left = ui.available_rect_before_wrap();

                            // Draw left side (virtual view)
                            let factor_fit_x = (props.resolution[0] as f32 * left.height() / props.resolution[1] as f32 / left.width()).min(1.);
                            let factor_fit_y = (props.resolution[1] as f32 * left.width() / props.resolution[0] as f32 / left.height()).min(1.);
                            let left = Rect::from_center_size(left.center(), left.size() * vec2(factor_fit_x, factor_fit_y));

                            // Draw the preview image
                            let uv = Rect::from_min_max(pos2(0.0, 0.0), pos2(1.0, 1.0));
                            ui.painter()
                                .add(Shape::image(preview_image, left, uv, Color32::DARK_GRAY));

                            // Draw the crop bounds
                            for (i, screen) in props.screens.iter().enumerate() {
                                let inv_map = screen.map.try_inverse().unwrap(); // Calling unwrap on user data

                                let uv_to_pt = |uv: Vector2<f32>| {
                                    // Input UV is in physical space.
                                    // Multiply it by the inverse map matrix to get it in virtual space.
                                    let uvw = Vector3::<f32>::new(uv[0], uv[1], 1.);
                                    let uvw = inv_map * uvw;

                                    left.min + vec2(uvw[0] / uvw[2], uvw[1] / uvw[2]) * (left.max - left.min)
                                };

                                for (j, vertex) in screen.crop.iter().enumerate() {
                                    let pt = uv_to_pt(*vertex);
                                    ui.painter().rect_stroke(Rect::from_center_size(pt, Vec2::splat(EDITOR_PT_SIZE)), 0., if i == selected_screen_index {EDITOR_STROKE_SELECTED} else {EDITOR_STROKE_BLURRED});
                                    let last_uv = screen.crop[if j > 0 {j - 1} else {screen.crop.len() - 1}];
                                    let last_pt = uv_to_pt(last_uv);
                                    ui.painter().line_segment([last_pt, pt], if i == selected_screen_index {EDITOR_STROKE_SELECTED} else {EDITOR_STROKE_BLURRED});
                                }
                            }

                            // Draw the outline
                            ui.painter().rect_stroke(left, 0., EDITOR_OUTLINE_STROKE);
                        }

                        {
                            let mut ui = ui.child_ui(right, Layout::top_down(Align::Center).with_cross_justify(true));
                            ui.heading("Physical:");

                            let right = ui.available_rect_before_wrap();
                            let screen = &props.screens[selected_screen_index];

                            // Draw right side (physical view)
                            let factor_fit_x = (screen.resolution[0] as f32 * right.height() / screen.resolution[1] as f32 / right.width()).min(1.);
                            let factor_fit_y = (screen.resolution[1] as f32 * right.width() / screen.resolution[0] as f32 / right.height()).min(1.);
                            let right = Rect::from_center_size(right.center(), right.size() * vec2(factor_fit_x, factor_fit_y));
                            let uv_to_pt = |uv| (right.min + uv * (right.max - right.min));

                            // Draw the crop bounds
                            for (i, vertex) in screen.crop.iter().enumerate() {
                                let pt = uv_to_pt(vec2(vertex[0], vertex[1]));
                                ui.painter().rect_stroke(Rect::from_center_size(pt, Vec2::splat(EDITOR_PT_SIZE)), 0., EDITOR_STROKE_SELECTED);
                                let last_uv = screen.crop[if i > 0 {i - 1} else {screen.crop.len() - 1}];
                                let last_pt = uv_to_pt(vec2(last_uv[0], last_uv[1]));
                                ui.painter().line_segment([last_pt, pt], EDITOR_STROKE_SELECTED);
                            }

                            // Draw the outline
                            ui.painter().rect_stroke(right, 0., EDITOR_OUTLINE_STROKE);
                        }
                });
            } else {
                // Node was deleted; close modal
                *modal_memory = None;
            }
        },
    }

    return ui.interact(Rect::NOTHING, id, Sense {click: false, drag: false, focusable: false});
}

pub fn modal<'a>(
    id: Id,
    props: &'a mut Props,
    node_states: &'a HashMap<NodeId, NodeState>,
    preview_images: &'a HashMap<NodeId, TextureId>,
) -> impl Widget + 'a {
    move |ui: &mut Ui| {
        modal_ui(
            ui,
            id,
            props,
            node_states,
            preview_images,
        )
    }
}
