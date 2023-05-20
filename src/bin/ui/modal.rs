use egui::{
    Ui, Widget, Sense, Rect, TextureId, Response, Id, Context, Layout, Align, Shape, Stroke, Color32
};
use std::sync::{Arc, Mutex};
use std::collections::{HashMap};
use radiance::{NodeId, NodeState, Props};

const SCRIM_FILL_COLOR: Color32 = Color32::BLACK;
const SCRIM_STROKE: Stroke = Stroke {
    width: 1.,
    color: Color32::GRAY,
};
const SCRIM_MARGIN: f32 = 30.;
const SCRIM_PADDING: f32 = 5.;

/// State associated with the modal UI being shown, to be preserved between frames.
#[derive(Debug)]
pub enum ModalMemory {
    ProjectionMapEditor, // TODO add node ID
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
        ModalMemory::ProjectionMapEditor => {
            ui.heading("Projection Map Editor");
            if ui.button("Close").clicked() {
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
