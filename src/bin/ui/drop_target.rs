use eframe::egui::{vec2, Color32, Rect, Response, Sense, Ui, Widget};

const FILL_ACTIVE: Color32 = Color32::from_rgba_premultiplied(102, 0, 170, 200);
const DROP_TARGET_SIZE: f32 = 30.;

/// A UI element representing a graphical drop target
/// e.g. the space between the connectors on two tiles
/// (not stateful; builder pattern)
#[derive(Debug, Clone)]
pub struct DropTarget {
    rect: Rect, // rect contains this drop target's laid-out spot in the mosaic
}

impl DropTarget {
    pub fn new(rect: Rect, _active: bool) -> Self {
        Self { rect }
    }

    pub fn rect(&self) -> Rect {
        self.rect
    }

    pub fn with_rect(mut self, rect: Rect) -> Self {
        self.rect = rect;
        self
    }

    fn paint(&self, ui: &Ui) {
        let painted_rect =
            Rect::from_center_size(self.rect.center(), vec2(DROP_TARGET_SIZE, DROP_TARGET_SIZE));
        if !ui.is_rect_visible(painted_rect) {
            return;
        }

        ui.painter()
            .rect_filled(painted_rect, 0.5 * DROP_TARGET_SIZE, FILL_ACTIVE);
    }
}

impl Widget for DropTarget {
    fn ui(self, ui: &mut Ui) -> Response {
        let response = ui.allocate_rect(self.rect, Sense::hover());

        if response.hovered() {
            self.paint(ui);
        }

        response
    }
}
