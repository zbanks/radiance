use egui::{Ui, RichText, TextureId, Response, vec2, Layout, Align, Slider, Widget, InnerResponse};
use crate::ui::tile::Tile;
use radiance::{EffectNodeProps, EffectNodeState};

const PREVIEW_ASPECT_RATIO: f32 = 1.;
const NORMAL_HEIGHT: f32 = 200.;
const NORMAL_WIDTH: f32 = 120.;

pub struct EffectNodeTile<'a> {
    title: RichText,
    preview_image: TextureId,
    intensity: &'a mut f32,
}

impl<'a> EffectNodeTile<'a> {
    /// Returns a Vec with one entry for each props.input_count
    /// corresponding to the minimum allowable height for that input port.
    /// If there are no input ports, this function should return a 1-element Vec.
    pub fn min_input_heights(props: &EffectNodeProps) -> Vec<f32> {
        (0..1.max(props.input_count)).map(|_| NORMAL_HEIGHT).collect()
    }

    /// Calculates the width of the tile, given its height.
    pub fn width_for_height(props: &EffectNodeProps, height: f32) -> f32 {
        NORMAL_WIDTH.min(0.5 * height)
    }

    /// Creates a new visual tile
    /// (builder pattern; this is not a stateful UI component)
    pub fn new(props: &'a mut EffectNodeProps, _state: &'a EffectNodeState, preview_image: TextureId) -> Self {
        EffectNodeTile {
            title: (&props.name).into(),
            preview_image,
            intensity: &mut props.intensity,
        }
    }

    /// Render the contents of the EffectNodeTile (presumably into a Tile)
    pub fn add_contents(self, ui: &mut Ui) {
        let EffectNodeTile {title, preview_image, intensity} = self;
        ui.heading(title);
        // Preserve aspect ratio
        ui.with_layout(Layout::bottom_up(Align::Center).with_cross_justify(true), |ui| {
            ui.spacing_mut().slider_width = ui.available_width();
            ui.add(Slider::new(intensity, 0.0..=1.0).show_value(false));
            ui.centered_and_justified(|ui| {
                let image_size = ui.available_size();
                let image_size = (image_size * vec2(1., 1. / PREVIEW_ASPECT_RATIO)).min_elem() * vec2(1., PREVIEW_ASPECT_RATIO);
                ui.image(preview_image, image_size);
            });
        });
    }
}
