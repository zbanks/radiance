use eframe::egui::{
    pos2, vec2, Align, Color32, ComboBox, Layout, Rect, RichText, Shape, Slider, Spinner,
    TextureId, Ui, UiBuilder,
};
use radiance::{EffectNodeProps, EffectNodeState};

const PREVIEW_ASPECT_RATIO: f32 = 1.;
const NORMAL_HEIGHT: f32 = 200.;
const NORMAL_WIDTH: f32 = 150.;

const SCRIM: Color32 = Color32::from_rgba_premultiplied(144, 144, 144, 230);
const ICON: Color32 = Color32::from_rgb(102, 0, 170);

pub enum EffectNodeTileState {
    Initializing,
    Ready,
    Error,
}

pub struct EffectNodeTile<'a> {
    title: RichText,
    preview_image: TextureId,
    intensity: &'a mut Option<f32>, // TODO turn these Options into a more holistic enum based on EffectNodeState
    frequency: &'a mut Option<f32>,
    state: EffectNodeTileState,
}

impl<'a> EffectNodeTile<'a> {
    /// Returns a Vec with one entry for each props.input_count
    /// corresponding to the minimum allowable height for that input port.
    /// If there are no input ports, this function should return a 1-element Vec.
    pub fn min_input_heights(props: &EffectNodeProps) -> Vec<f32> {
        // TODO Simplify this to just be a single f32
        (0..1.max(props.input_count.unwrap_or(1)))
            .map(|_| NORMAL_HEIGHT)
            .collect()
    }

    /// Calculates the width of the tile, given its height.
    pub fn width_for_height(_props: &EffectNodeProps, height: f32) -> f32 {
        NORMAL_WIDTH.min(0.6 * height)
    }

    /// Creates a new visual tile
    /// (builder pattern; this is not a stateful UI component)
    pub fn new(
        props: &'a mut EffectNodeProps,
        state: &'a EffectNodeState,
        preview_image: TextureId,
    ) -> Self {
        let tile_state = match state {
            EffectNodeState::Uninitialized => EffectNodeTileState::Initializing,
            EffectNodeState::Ready(_) => EffectNodeTileState::Ready,
            EffectNodeState::Error_(_) => EffectNodeTileState::Error,
        };

        EffectNodeTile {
            title: (&props.name).into(),
            preview_image,
            intensity: &mut props.intensity,
            frequency: &mut props.frequency,
            state: tile_state,
        }
    }

    /// Render the contents of the EffectNodeTile (presumably into a Tile)
    pub fn add_contents(self, ui: &mut Ui) {
        let EffectNodeTile {
            title,
            preview_image,
            intensity,
            frequency,
            state,
        } = self;
        ui.heading(title);
        // Preserve aspect ratio
        ui.with_layout(
            Layout::bottom_up(Align::Center).with_cross_justify(true),
            |ui| {
                ui.spacing_mut().slider_width = ui.available_width();
                intensity
                    .as_mut()
                    .map(|intensity| ui.add(Slider::new(intensity, 0.0..=1.0).show_value(false)));

                frequency.as_mut().map(|frequency| {
                    let frequencies: &[f32] = &[0., 0.125, 0.25, 0.5, 1., 2., 4., 8.];
                    fn str_for_frequency(frequency: f32) -> String {
                        if frequency > 0. && frequency < 1. {
                            format!("1/{}", 1. / frequency)
                        } else {
                            format!("{}", frequency)
                        }
                    }
                    ComboBox::from_id_salt("frequency")
                        .width(ui.available_width())
                        .selected_text(format!("f={}", str_for_frequency(*frequency)).as_str())
                        .show_ui(ui, |ui| {
                            for &option in frequencies.iter() {
                                ui.selectable_value(
                                    frequency,
                                    option,
                                    str_for_frequency(option).as_str(),
                                );
                            }
                        });
                });

                ui.horizontal_centered(|ui| {
                    let image_size = ui.available_size();
                    let image_size = (image_size * vec2(1., 1. / PREVIEW_ASPECT_RATIO)).min_elem()
                        * vec2(1., PREVIEW_ASPECT_RATIO);
                    let (_, image_rect) = ui.allocate_space(image_size);
                    let uv = Rect::from_min_max(pos2(0.0, 0.0), pos2(1.0, 1.0));
                    ui.painter()
                        .add(Shape::image(preview_image, image_rect, uv, Color32::WHITE));
                    let image_rect = image_rect.expand(1.); // Sometimes the preview creeps out from under the scrim
                    match state {
                        EffectNodeTileState::Error => {
                            // Show scrim and ! icon if node is in an error state
                            ui.painter().add(Shape::rect_filled(image_rect, 0., SCRIM));
                            ui.scope_builder(UiBuilder::default().max_rect(image_rect), |ui| {
                                ui.centered_and_justified(|ui| {
                                    ui.label(
                                        RichText::new("!").color(ICON).size(0.9 * image_size.y),
                                    );
                                });
                            });
                        }
                        EffectNodeTileState::Initializing => {
                            // Show scrim and spinner if node is loading
                            ui.painter().add(Shape::rect_filled(image_rect, 0., SCRIM));
                            ui.scope_builder(UiBuilder::default().max_rect(image_rect), |ui| {
                                ui.centered_and_justified(|ui| {
                                    ui.add(Spinner::new().size(0.5 * image_size.y).color(ICON));
                                });
                            });
                        }
                        EffectNodeTileState::Ready => {}
                    }
                });
            },
        );
    }
}
