use egui::{Ui, RichText, TextureId, Response, vec2, Layout, Align, Slider, Widget, InnerResponse};
use crate::ui::video_node_tile::VideoNodeTile;

const PREVIEW_ASPECT_RATIO: f32 = 1.;

pub struct EffectNodeTile<'a> {
    tile: VideoNodeTile<'a>,
    title: RichText,
    preview_image: TextureId,
    intensity: &'a mut f32,
}

impl<'a> EffectNodeTile<'a> {
    pub fn new(tile: VideoNodeTile<'a>, title: impl Into<RichText>, preview_image: TextureId, intensity: &'a mut f32) -> Self {
        EffectNodeTile {
            tile,
            title: title.into(),
            preview_image,
            intensity,
        }
    }
}

impl<'a> Widget for EffectNodeTile<'a> {
    fn ui(self, ui: &mut Ui) -> Response {
        let EffectNodeTile {tile, title, preview_image, intensity} = self;
        let InnerResponse {inner: _, response} = tile.show(ui, |ui| {
            ui.heading(title);
            // Preserve aspect ratio
            ui.with_layout(Layout::bottom_up(Align::Center).with_cross_justify(true), |ui| {
                //ui.add(Slider::new(intensity, 0.0..=1.0).show_value(false)); // TODO why does this slider not fill the horizontal space?
                ui.centered_and_justified(|ui| {
                    let image_size = ui.available_size();
                    let image_size = (image_size * vec2(1., 1. / PREVIEW_ASPECT_RATIO)).min_elem() * vec2(1., PREVIEW_ASPECT_RATIO);
                    ui.image(preview_image, image_size);
                });
            });
        });
        response
    }
}
