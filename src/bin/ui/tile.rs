use radiance::NodeId;
use egui::{Vec2, Ui, Sense, Layout, Align, InnerResponse, Color32, Stroke, Rect, Response, TextureId, Mesh, Pos2, Shape, pos2, vec2};

/// A unique identifier for a visual tile in the UI.
/// There may be multiple tiles per node,
/// since the graph may be a DAG but is visualized as a tree.
/// The "instance" field increments for each additional tile for the same NodeId.
#[derive(Debug, Eq, Hash, PartialEq, Clone, Copy)]
pub struct TileId {
    pub node: NodeId,
    pub instance: u32,
}

pub struct Tile {
    id: TileId,
    rect: Rect,
    inputs: Vec<f32>,
    outputs: Vec<f32>,
    selected: bool,
}

fn cross(a: Vec2, b: Vec2) -> f32 {
    a.x * b.y - a.y * b.x
}

const FILL_DESELECTED: Color32 = Color32::BLACK;
const FILL_SELECTED: Color32 = Color32::from_rgb(34, 0, 57);
const STROKE_FOCUSED: Stroke = Stroke {
    width: 2.,
    color: Color32::LIGHT_GRAY,
};
const STROKE_BLURRED: Stroke = Stroke {
    width: 1.,
    color: Color32::GRAY,
};

const MARGIN_HORIZONTAL: f32 = 20.;
const MARGIN_VERTICAL: f32 = 10.;
const CHEVRON_SIZE: f32 = 15.;
const EPSILON: f32 = 0.0001;

impl Tile {
   pub fn new(id: TileId, rect: Rect, inputs: Vec<f32>, outputs: Vec<f32>) -> Self {
        Self {
            id,
            rect,
            inputs,
            outputs,
            selected: false,
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
        let mut content_ui = ui.child_ui(self.rect.shrink2(vec2(MARGIN_HORIZONTAL, MARGIN_VERTICAL)), Layout::top_down(Align::Center));
        let inner = add_contents(&mut content_ui);
        InnerResponse::new(inner, response)
    }

    fn paint(&self, ui: &Ui, response: &Response) {
        if !ui.is_rect_visible(self.rect.expand(CHEVRON_SIZE)) {
            return;
        }

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

        let input_sizes = calc_chevron_sizes(&self.inputs);
        let output_sizes = calc_chevron_sizes(&self.outputs);

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
        let fill = if self.selected { FILL_SELECTED } else { FILL_DESELECTED };
        let mut mesh = Mesh::with_texture(TextureId::default());
        for &pos in vertices.iter() {
            mesh.colored_vertex(pos, fill);
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
        let stroke = if response.has_focus() { STROKE_FOCUSED } else { STROKE_BLURRED };
        ui.painter().add(Shape::closed_line(vertices, stroke));
    }
}
