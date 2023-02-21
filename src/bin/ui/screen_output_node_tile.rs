use egui::{Ui, TextureId, vec2, Layout, Align, Checkbox};
use radiance::{ScreenOutputNodeProps, ScreenOutputNodeState, SelectedOutputScreen, AvailableOutputScreen};

const PREVIEW_ASPECT_RATIO: f32 = 1.;
const NORMAL_HEIGHT: f32 = 300.;
const NORMAL_WIDTH: f32 = 220.;

pub struct ScreenOutputNodeTile<'a> {
    preview_image: TextureId,
    visible: &'a mut bool,
    screen: &'a mut Option<SelectedOutputScreen>,
    available_screens: &'a [AvailableOutputScreen],
}

impl<'a> ScreenOutputNodeTile<'a> {
    /// Returns a Vec with one entry for each props.input_count
    /// corresponding to the minimum allowable height for that input port.
    /// If there are no input ports, this function should return a 1-element Vec.
    pub fn min_input_heights(props: &ScreenOutputNodeProps) -> Vec<f32> {
        // TODO Simplify this to just be a single f32
        (0..1).map(|_| NORMAL_HEIGHT).collect()
    }

    /// Calculates the width of the tile, given its height.
    pub fn width_for_height(props: &ScreenOutputNodeProps, height: f32) -> f32 {
        NORMAL_WIDTH.min(0.5 * height)
    }

    /// Creates a new visual tile
    /// (builder pattern; this is not a stateful UI component)
    pub fn new(props: &'a mut ScreenOutputNodeProps, _state: &'a ScreenOutputNodeState, preview_image: TextureId) -> Self {
        ScreenOutputNodeTile {
            preview_image,
            visible: &mut props.visible,
            screen: &mut props.screen,
            available_screens: &props.available_screens,
        }
    }

    /// Render the contents of the ScreenOutputNodeTile (presumably into a Tile)
    pub fn add_contents(self, ui: &mut Ui) {
        let ScreenOutputNodeTile {preview_image, visible, screen, available_screens} = self;

        ui.heading("Screen Output");
        // Preserve aspect ratio
        ui.with_layout(Layout::bottom_up(Align::Center).with_cross_justify(true), |ui| {
            ui.add(Checkbox::new(visible, "Visible"));

            let (mut screen_name, mut screen_resolution) = match screen.as_ref() {
                Some(screen) => (screen.name.clone(), screen.resolution.clone()),
                None => ((String::new(), [0, 0]))
            };

            let available_resolutions: Vec<[u32; 2]> = screen.as_ref().map(|screen| {
                available_screens.iter().find(|available_screen| available_screen.name == screen.name)
                .map(|selected_available_screen| selected_available_screen.suggested_resolutions.iter().cloned())
            }).into_iter().flatten().flatten().collect();

            let format_rez = |rez: &[u32; 2]| format!("{}x{}", rez[0], rez[1]);

            egui::ComboBox::from_id_source(1)
                .selected_text(format_rez(&screen_resolution))
                .show_ui(ui, |ui| {
                    for available_resolution in available_resolutions.iter() {
                        ui.selectable_value(&mut screen_resolution, available_resolution.clone(), format_rez(&available_resolution));
                    }
                }
            );
            egui::ComboBox::from_id_source(0)
                .selected_text(screen_name.as_str())
                .show_ui(ui, |ui| {
                    for available_screen in available_screens.iter() {
                        ui.selectable_value(&mut screen_name, available_screen.name.clone(), available_screen.name.as_str());
                    }
                }
            );

            if !screen_name.is_empty() && screen_resolution != [0, 0] {
                *screen = Some(SelectedOutputScreen {
                    name: screen_name,
                    resolution: screen_resolution,
                });
            }

            ui.centered_and_justified(|ui| {
                let image_size = ui.available_size();
                let image_size = (image_size * vec2(1., 1. / PREVIEW_ASPECT_RATIO)).min_elem() * vec2(1., PREVIEW_ASPECT_RATIO);
                ui.image(preview_image, image_size);
            });
        });
    }
}
