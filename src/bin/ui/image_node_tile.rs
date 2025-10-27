use eframe::egui::{vec2, Align, ComboBox, Layout, RichText, TextureId, Ui};
use radiance::{Fit, ImageNodeProps, ImageNodeState};

const PREVIEW_ASPECT_RATIO: f32 = 1.;
const NORMAL_HEIGHT: f32 = 200.;
const NORMAL_WIDTH: f32 = 120.;

pub struct ImageNodeTile<'a> {
    title: RichText,
    preview_image: TextureId,
    fit: &'a mut Option<Fit>,
}

impl<'a> ImageNodeTile<'a> {
    /// Returns a Vec with one entry for each props.input_count
    /// corresponding to the minimum allowable height for that input port.
    /// If there are no input ports, this function should return a 1-element Vec.
    pub fn min_input_heights(_props: &ImageNodeProps) -> Vec<f32> {
        // TODO Simplify this to just be a single f32
        (0..1).map(|_| NORMAL_HEIGHT).collect()
    }

    /// Calculates the width of the tile, given its height.
    pub fn width_for_height(_props: &ImageNodeProps, height: f32) -> f32 {
        NORMAL_WIDTH.min(0.5 * height)
    }

    /// Creates a new visual tile
    /// (builder pattern; this is not a stateful UI component)
    pub fn new(
        props: &'a mut ImageNodeProps,
        _state: &'a ImageNodeState,
        preview_image: TextureId,
    ) -> Self {
        ImageNodeTile {
            title: (&props.name).into(),
            preview_image,
            fit: &mut props.fit,
        }
    }

    /// Render the contents of the ImageNodeTile (presumably into a Tile)
    pub fn add_contents(self, ui: &mut Ui) {
        let ImageNodeTile {
            title,
            preview_image,
            fit,
        } = self;
        ui.heading(title);
        // Preserve aspect ratio
        ui.with_layout(
            Layout::bottom_up(Align::Center).with_cross_justify(true),
            |ui| {
                if let Some(fit) = fit {
                    ComboBox::from_id_salt("fit")
                        .selected_text(match fit {
                            Fit::Crop => "Crop",
                            Fit::Shrink => "Shrink",
                            Fit::Zoom => "Zoom",
                        })
                        .width(ui.available_size().x)
                        .show_ui(ui, |ui| {
                            ui.selectable_value(fit, Fit::Crop, "Crop");
                            ui.selectable_value(fit, Fit::Shrink, "Shrink");
                            ui.selectable_value(fit, Fit::Zoom, "Zoom");
                        });
                }

                ui.centered_and_justified(|ui| {
                    let image_size = ui.available_size();
                    let image_size = (image_size * vec2(1., 1. / PREVIEW_ASPECT_RATIO)).min_elem()
                        * vec2(1., PREVIEW_ASPECT_RATIO);
                    ui.image((preview_image, image_size));
                });
            },
        );
    }
}
