use egui::{Id, Vec2, Ui, vec2, Sense, Layout, Align, InnerResponse, Color32, Stroke, Rect};
use std::hash::Hash;

pub struct VideoNodeTile<'a> {
    id: Id,
    rect: Rect,
    selected: bool,
    inputs: &'a [f32],
    outputs: &'a [f32],
}

const MARGIN: f32 = 10.;

impl<'a> VideoNodeTile<'a> {
   pub fn new(id_source: impl Hash, rect: Rect, inputs: &'a [f32], outputs: &'a [f32]) -> Self {
        Self {
            id: Id::new(id_source),
            rect,
            selected: false,
            inputs,
            outputs,
        }
    }

    pub fn selected(mut self, selected: bool) -> Self {
        self.selected = selected;
        self
    }

    pub fn show<R>(self, ui: &mut Ui, add_contents: impl FnOnce(&mut Ui) -> R) -> InnerResponse<R> {
        self.show_dyn(ui, Box::new(add_contents))
    }

    fn show_dyn<'c, R>(
        self,
        ui: &mut Ui,
        add_contents: Box<dyn FnOnce(&mut Ui) -> R + 'c>,
    ) -> InnerResponse<R> {
        let response = ui.allocate_rect(self.rect, Sense::click());
        if response.clicked() {
            response.request_focus();
        }
        if ui.is_rect_visible(self.rect) {
            //let visuals = ui.style().interact_selectable(&response, self.selected);
           ui.painter().rect(self.rect, 0., Color32::BLACK, Stroke::new(2., if response.has_focus() {Color32::LIGHT_GRAY} else {Color32::GRAY}));
        }
        let mut content_ui = ui.child_ui_with_id_source(self.rect.shrink(MARGIN), Layout::top_down_justified(Align::Center), self.id);
        let inner = add_contents(&mut content_ui);
        InnerResponse::new(inner, response)
    }
}
