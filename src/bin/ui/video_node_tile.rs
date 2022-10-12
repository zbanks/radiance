use egui::{Id, Vec2, Ui, vec2, Sense, Layout, Align, InnerResponse, Color32, Stroke, Rect, Response, pos2, TextureId, Mesh, Pos2, Shape};
use std::hash::Hash;

pub struct VideoNodeTile<'a> {
    id: Id,
    rect: Rect,
    selected: bool,
    inputs: &'a [f32],
    outputs: &'a [f32],
}

fn cross(a: Vec2, b: Vec2) -> f32 {
    a.x * b.y - a.y * b.x
}

const MARGIN: f32 = 10.;
const BORDER_THICKNESS: f32 = 2.;
const CHEVRON_SIZE: f32 = 15.;
const EPSILON: f32 = 0.0001;

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
        self.paint(&ui, &response);
        //if ui.is_rect_visible(self.rect) {
            //let visuals = ui.style().interact_selectable(&response, self.selected);
           //ui.painter().rect(self.rect, 0., Color32::BLACK, Stroke::new(2., if response.has_focus() {Color32::LIGHT_GRAY} else {Color32::GRAY}));
        //}
        let mut content_ui = ui.child_ui_with_id_source(self.rect.shrink(MARGIN), Layout::top_down_justified(Align::Center), self.id);
        let inner = add_contents(&mut content_ui);
        InnerResponse::new(inner, response)
    }

    fn paint(&self, ui: &Ui, response: &Response) {
        // Figure out how big each chevron can be without interfering with the
        // rectangle boundaries or other chevrons
        let calc_chevron_sizes = |locations: &[f32]| {
            (0..locations.len()).map(|i| {
                let pos = locations[i];
                let left_boundary = if i == 0 {
                    0.
                } else {
                    0.5 * (locations[i - 1] + locations[i])
                };
                let right_boundary = if i == locations.len() - 1 {
                    self.rect.height()
                } else {
                    0.5 * (locations[i] + locations[i + 1])
                };

                CHEVRON_SIZE.min(pos - left_boundary).min(right_boundary - pos).max(0.)
            }).collect::<Vec<f32>>()
        };

        let input_sizes = calc_chevron_sizes(self.inputs);
        let output_sizes = calc_chevron_sizes(self.outputs);

        // Construct the polygon in CCW order starting with the top left
        let mut vertices = Vec::<Pos2>::new();
        vertices.push(pos2(self.rect.left(), self.rect.top()));
        let mut last_loc = *self.inputs.first().unwrap_or(&f32::NAN);
        for (&loc, &size) in self.inputs.iter().zip(input_sizes.iter()) {
            assert!(loc >= last_loc, "Inputs are not sorted");
            if size > 0. {
                vertices.push(pos2(self.rect.left(), self.rect.top() + loc - size));
                vertices.push(pos2(self.rect.left() + size, self.rect.top() + loc));
                vertices.push(pos2(self.rect.left(), self.rect.top() + loc + size));
            }
            last_loc = loc;
        }
        vertices.push(pos2(self.rect.left(), self.rect.bottom()));
        vertices.push(pos2(self.rect.right(), self.rect.bottom()));
        let mut last_loc = *self.outputs.last().unwrap_or(&f32::NAN);
        for (&loc, &size) in self.outputs.iter().rev().zip(output_sizes.iter().rev()) {
            assert!(loc <= last_loc, "Outputs are not sorted");
            if size > 0. {
                vertices.push(pos2(self.rect.right(), self.rect.top() + loc + size));
                vertices.push(pos2(self.rect.right() + size, self.rect.top() + loc));
                vertices.push(pos2(self.rect.right(), self.rect.top() + loc - size));
            }
            last_loc = loc;
        }
        vertices.push(pos2(self.rect.right(), self.rect.top()));

        vertices.dedup_by(|a, b| (a.x - b.x).abs() < EPSILON && (a.y - b.y).abs() < EPSILON);

        // Push vertices of polygon
        let mut mesh = Mesh::with_texture(TextureId::default());
        for &pos in vertices.iter() {
            mesh.colored_vertex(pos, Color32::BLACK);
        }

        // Triangulate with ear clipping
        // (egui doesn't support concave polygons)
        let mut indices: Vec<usize> = (0..vertices.len()).collect();
        while indices.len() > 2 {
            // Find an ear
            let mut found_interior = false;
            let mut found_exterior = false;
            for i in 0..indices.len() {
                let ii1 = (indices.len() + i - 1) % indices.len();
                let ii2 = i;
                let ii3 = (i + 1) % indices.len();
                let p1 = vertices[indices[ii1]];
                let p2 = vertices[indices[ii2]];
                let p3 = vertices[indices[ii3]];
                let c = cross(p2 - p1, p3 - p2);
                if c > EPSILON {
                    found_exterior = true;
                } else if c < -EPSILON {
                    // CCW polygon winding produces negative cross products on interior edges
                    found_interior = true;
                    mesh.add_triangle((indices[ii1]) as u32, (indices[ii2]) as u32, (indices[ii3]) as u32);
                    indices.remove(i); // Clip
                    break;
                }
            }
            if !found_interior && !found_exterior {
                // We have clipped the polygon to the point that it is degenerate
                // (has no area.)
                // I'd call that a success.
                break;
            }
            assert!(found_interior || !found_exterior, "Triangulation failed (malformed polygon?)");
        }
        // Paint fill
        ui.painter().add(mesh);

        // Paint stroke
        let stroke = Stroke::new(BORDER_THICKNESS, if response.has_focus() {Color32::LIGHT_GRAY} else {Color32::GRAY});
        ui.painter().add(Shape::closed_line(vertices, stroke));
    }
}
