use egui::{vec2, Align, Checkbox, Layout, TextureId, Ui, Id};
use crate::ui::{set_modal, ModalMemory};
use radiance::{
    AvailableOutputScreen, ProjectionMappedOutputNodeProps, ProjectionMappedOutputNodeState,
};

const PREVIEW_ASPECT_RATIO: f32 = 1.;
const NORMAL_HEIGHT: f32 = 300.;
const NORMAL_WIDTH: f32 = 220.;

pub struct ProjectionMappedOutputNodeTile<'a> {
    preview_image: TextureId,
    visible: &'a mut bool,
    available_screens: &'a [AvailableOutputScreen],
    modal_id: Id,
}

impl<'a> ProjectionMappedOutputNodeTile<'a> {
    /// Returns a Vec with one entry for each props.input_count
    /// corresponding to the minimum allowable height for that input port.
    /// If there are no input ports, this function should return a 1-element Vec.
    pub fn min_input_heights(_props: &ProjectionMappedOutputNodeProps) -> Vec<f32> {
        // TODO Simplify this to just be a single f32
        (0..1).map(|_| NORMAL_HEIGHT).collect()
    }

    /// Calculates the width of the tile, given its height.
    pub fn width_for_height(_props: &ProjectionMappedOutputNodeProps, height: f32) -> f32 {
        NORMAL_WIDTH.min(0.5 * height)
    }

    /// Creates a new visual tile
    /// (builder pattern; this is not a stateful UI component)
    pub fn new(
        props: &'a mut ProjectionMappedOutputNodeProps,
        _state: &'a ProjectionMappedOutputNodeState,
        preview_image: TextureId,
        modal_id: Id,
    ) -> Self {
        ProjectionMappedOutputNodeTile {
            preview_image,
            visible: &mut props.visible,
            available_screens: &props.available_screens,
            modal_id,
        }
    }

    /// Render the contents of the ProjectionMappedOutputNodeTile (presumably into a Tile)
    pub fn add_contents(self, ui: &mut Ui) {
        let ProjectionMappedOutputNodeTile {
            preview_image,
            visible,
            available_screens,
            modal_id,
        } = self;

        ui.heading("Projection Mapped Output");
        // Preserve aspect ratio
        ui.with_layout(
            Layout::bottom_up(Align::Center).with_cross_justify(true),
            |ui| {
                ui.add(Checkbox::new(visible, "Visible"));

                if ui.button("Edit...").clicked() {
                    set_modal(ui.ctx(), modal_id, Some(ModalMemory::ProjectionMapEditor));
                }

                ui.centered_and_justified(|ui| {
                    let image_size = ui.available_size();
                    let image_size = (image_size * vec2(1., 1. / PREVIEW_ASPECT_RATIO)).min_elem()
                        * vec2(1., PREVIEW_ASPECT_RATIO);
                    ui.image(preview_image, image_size);
                });
            },
        );
    }
}
