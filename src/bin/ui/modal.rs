use egui::{
    Ui, Widget, Sense, Rect, TextureId, Response, Id, Context, Layout, Align, Shape, Stroke, Color32, pos2, vec2, Vec2
};
use std::sync::{Arc, Mutex};
use std::collections::{HashMap};
use radiance::{NodeId, NodeState, Props, NodeProps};
use nalgebra::{Vector2, Vector3, Matrix3, SVector, SMatrix};

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
    ProjectionMapEditor(NodeId),
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

/// Find the 3x3 transformation matrix such that M * A1 = B1, M * A2 = B2, etc.
#[rustfmt::skip]
fn four_point_mapping(a: [Vector2<f32>; 4], b: [Vector2<f32>; 4]) -> Option<Matrix3<f32>> {
    // We will operate in homogeneous coordinates.
    // a_n becomes [a_n_x, a_n_y, 1] and b_n becomes [b_n_x, b_n_y, 1] * k_n
    // and we will fix M_ww to 1

    // Solve the system of 12 linear equations:
    // M_xx * A_n_x + M_xy * A_n_y + M_xw - k_n * B_n_x = 0
    // M_yx * A_n_x + M_yy * A_n_y + M_yw - k_n * B_n_y = 0
    // M_wx * A_n_x + M_wy * A_n_y        - k_n         = -1

    // in the form:
    // s_a * s_x = s_b

    // Variables vector:
    // s_x = [M_xx, M_xy, M_xw, M_yx, M_yy, M_yw, M_wx, M_wy, k1, k2, k3, k4]

    // System of equations:
    let s_a = SMatrix::<f32, 12, 12>::from_row_slice(&[
        // [M_xx,    M_xy,    M_xw,    M_yx,    M_yy,    M_yw,    M_wx,    M_wy,    k1,       k2,       k3,       k4]
            a[0][0], a[0][1], 1.,      0.,      0.,      0.,      0.,      0.,      -b[0][0], 0.,       0.,       0.,
            0.,      0.,      0.,      a[0][0], a[0][1], 1.,      0.,      0.,      -b[0][1], 0.,       0.,       0.,
            0.,      0.,      0.,      0.,      0.,      0.,      a[0][0], a[0][1], -1.,      0.,       0.,       0.,
            a[1][0], a[1][1], 1.,      0.,      0.,      0.,      0.,      0.,      0.,       -b[1][0], 0.,       0.,  
            0.,      0.,      0.,      a[1][0], a[1][1], 1.,      0.,      0.,      0.,       -b[1][1], 0.,       0.,  
            0.,      0.,      0.,      0.,      0.,      0.,      a[1][0], a[1][1], 0.,       -1.,      0.,       0.,  
            a[2][0], a[2][1], 1.,      0.,      0.,      0.,      0.,      0.,      0.,       0.,       -b[2][0], 0.,  
            0.,      0.,      0.,      a[2][0], a[2][1], 1.,      0.,      0.,      0.,       0.,       -b[2][1], 0.,  
            0.,      0.,      0.,      0.,      0.,      0.,      a[2][0], a[2][1], 0.,       0.,       -1.,      0.,  
            a[3][0], a[3][1], 1.,      0.,      0.,      0.,      0.,      0.,      0.,       0.,       0.,       -b[3][0],
            0.,      0.,      0.,      a[3][0], a[3][1], 1.,      0.,      0.,      0.,       0.,       0.,       -b[3][1],
            0.,      0.,      0.,      0.,      0.,      0.,      a[3][0], a[3][1], 0.,       0.,       0.,       -1.,  
    ]);

    let s_b = SVector::<f32, 12>::from_row_slice(&[0., 0., -1., 0., 0., -1., 0., 0., -1., 0., 0., -1.]);

    // s_x = s_a ^ -1 * s_b
    let s_x = s_a.try_inverse()? * s_b;

    Some(Matrix3::new(
        s_x[0], s_x[1], s_x[2],
        s_x[3], s_x[4], s_x[5],
        s_x[6], s_x[7], 1.,
    ))
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
            if let Some(NodeProps::ProjectionMappedOutputNode(props)) = props.node_props.get_mut(node_id) {
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
                            let mut ui = ui.child_ui_with_id_source(left, Layout::top_down(Align::Center).with_cross_justify(true), "virtual");
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
                            for (i, screen) in props.screens.iter_mut().enumerate() {
                                let virtual_to_physical = screen.map;
                                let physical_to_virtual = virtual_to_physical.try_inverse().unwrap(); // Calling unwrap on user data
                                let virtual_uv_to_pt = Matrix3::<f32>::new(
                                    left.max.x - left.min.x, 0., left.min.x,
                                    0., left.max.y - left.min.y, left.min.y,
                                    0., 0., 1.);
                                let pt_to_virtual_uv = virtual_uv_to_pt.try_inverse().unwrap();

                                // Convert a point in physical UV space
                                // to egui pt coordinates
                                let physical_uv_to_pt = |uv: Vector2<f32>| {
                                    let uvw = Vector3::<f32>::new(uv[0], uv[1], 1.);
                                    let pt = virtual_uv_to_pt * physical_to_virtual * uvw;
                                    pos2(pt[0] / pt[2], pt[1] / pt[2])
                                };

                                // Convert a vector in egui pt coordiantes
                                // to a vector in virtual UV space
                                let delta_pt_to_delta_virtual_uv = |delta_pt: Vec2| {
                                    let delta_pt = Vector3::<f32>::new(delta_pt.x, delta_pt.y, 0.);
                                    let delta_uvw = pt_to_virtual_uv * delta_pt;
                                    Vector2::<f32>::new(delta_uvw[0], delta_uvw[1])
                                };

                                // Convert a point physical UV space
                                // to a point in virtual UV space
                                let physical_uv_to_virtual_uv = |physical_uv: Vector2<f32>| {
                                    let uvw = Vector3::<f32>::new(physical_uv[0], physical_uv[1], 1.);
                                    let uvw = physical_to_virtual * uvw;
                                    Vector2::<f32>::new(uvw[0] / uvw[2], uvw[1] / uvw[2])
                                };

                                let mut handle_drag = None;
                                for (j, vertex) in screen.crop.iter().enumerate() {
                                    let pt = physical_uv_to_pt(*vertex);
                                    let handle_rect = Rect::from_center_size(pt, Vec2::splat(EDITOR_PT_SIZE));

                                    let handle_response = ui.interact(handle_rect, Id::new(("virtual handle", j)), Sense::click_and_drag());
                                    if handle_response.dragged() {
                                        let delta_pt = handle_response.drag_delta();
                                        let delta_uv = delta_pt_to_delta_virtual_uv(delta_pt);
                                        handle_drag = Some((j, delta_uv));
                                    }

                                    ui.painter().rect_stroke(handle_rect, 0., if i == selected_screen_index {EDITOR_STROKE_SELECTED} else {EDITOR_STROKE_BLURRED});
                                    let last_uv = screen.crop[if j > 0 {j - 1} else {screen.crop.len() - 1}];
                                    let last_pt = physical_uv_to_pt(last_uv);
                                    ui.painter().line_segment([last_pt, pt], if i == selected_screen_index {EDITOR_STROKE_SELECTED} else {EDITOR_STROKE_BLURRED});
                                }

                                // Handle dragging of handles
                                if let Some((handle_index, delta_uv)) = handle_drag {
                                    let locked_point_indices = [0, 1, 2, 3];

                                    // Get the locked physical UVs that we don't want these to move
                                    let locked_physical_uvs = locked_point_indices.map(|j| screen.crop[j]);

                                    // Convert physical to virtual UVs and move the dragged one:
                                    let mut virtual_uvs: Vec<_> = screen.crop.iter().cloned().map(physical_uv_to_virtual_uv).collect();
                                    virtual_uvs[handle_index] += delta_uv;

                                    // Now get the locked virtual UVs post-drag:
                                    let locked_virtual_uvs = locked_point_indices.map(|j| virtual_uvs[j]);

                                    // Compute the new perspective matrix that keeps the locked physical points still
                                    let virtual_to_physical = four_point_mapping(locked_virtual_uvs, locked_physical_uvs).unwrap();
                                    screen.map = virtual_to_physical;

                                    // Re-compute the physical UVs from the virtual ones so all the virtual ones stay still
                                    screen.crop = virtual_uvs.into_iter().map(|virtual_uv: Vector2::<f32>| {
                                        let uvw = Vector3::<f32>::new(virtual_uv[0], virtual_uv[1], 1.);
                                        let uvw = virtual_to_physical * uvw;
                                        Vector2::<f32>::new(uvw[0] / uvw[2], uvw[1] / uvw[2])
                                    }).collect();
                                }
                            }

                            // Draw the outline
                            ui.painter().rect_stroke(left, 0., EDITOR_OUTLINE_STROKE);
                        }

                        {
                            let mut ui = ui.child_ui_with_id_source(right, Layout::top_down(Align::Center).with_cross_justify(true), "physical");
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
