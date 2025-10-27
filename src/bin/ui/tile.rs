use eframe::egui::{
    pos2, vec2, Align, Color32, Id, InnerResponse, Layout, Mesh, Pos2, Rect, Sense, Shape, Stroke,
    TextureId, Ui, UiBuilder, Vec2,
};
use radiance::NodeId;
use std::cmp::Ordering;

/// A unique identifier for a visual tile in the UI.
/// There may be multiple tiles per node,
/// since the graph may be a DAG but is visualized as a tree.
/// The "instance" field increments for each additional tile for the same NodeId.
#[derive(Debug, Eq, Hash, PartialEq, PartialOrd, Ord, Clone, Copy)]
pub struct TileId {
    pub node: NodeId,
    pub instance: u32,
}

/// A UI element representing an empty tile
/// (not stateful; builder pattern)
#[derive(Debug, Clone)]
pub struct Tile {
    id: TileId,
    ui_id: Id,
    rect: Rect,   // rect contains this tile's laid-out spot in the mosaic
    offset: Vec2, // offset contains any additional translation due to animation or drag
    inputs: Vec<f32>,
    outputs: Vec<f32>,
    focused: bool,
    selected: bool,
    lifted: bool,
    z: f32,
    alpha: f32,
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
const ALPHA_LIFTED: f32 = 0.3;

impl Tile {
    pub fn new(id: TileId, rect: Rect, inputs: Vec<f32>, outputs: Vec<f32>) -> Self {
        Self {
            id,
            ui_id: Id::new(id),
            rect,
            offset: Vec2::ZERO,
            inputs,
            outputs,
            focused: false,
            selected: false,
            lifted: false,
            z: 0.,
            alpha: 1.,
        }
    }

    pub fn with_focus(mut self, focused: bool) -> Self {
        self.focused = focused;
        self
    }

    pub fn with_selected(mut self, selected: bool) -> Self {
        self.selected = selected;
        self
    }

    pub fn with_lifted(mut self, lifted: bool) -> Self {
        self.lifted = lifted;
        self
    }

    pub fn with_offset(mut self, offset: Vec2) -> Self {
        self.offset = offset;
        self
    }

    pub fn with_rect(mut self, rect: Rect) -> Self {
        self.rect = rect;
        self
    }

    pub fn with_z(mut self, z: f32) -> Self {
        self.z = z;
        self
    }

    pub fn with_alpha(mut self, alpha: f32) -> Self {
        self.alpha = alpha;
        self
    }

    /// Set this tile's Z index based on whether it is lifted, and focused
    pub fn with_default_z(self) -> Self {
        let z = match (self.lifted, self.focused) {
            (false, false) => 0., // Normal tile in the mosaic
            (false, true) => 1., // Normal tile in the mosaic with focus (needs to be drawn above surrounding normal tiles)
            (true, false) => 2., // Tile is being dragged & dropped (needs to be drawn on top of the mosaic)
            (true, true) => 3., // Tile is being dragged & dropped and is focused (needs to be drawn on top of the surrounding lifted tiles)
        };
        self.with_z(z)
    }

    /// Set this tile's Z index based on whether it is lifted
    pub fn with_default_alpha(self) -> Self {
        let alpha = match self.lifted {
            false => 1.,          // Fully opaque if not lifted
            true => ALPHA_LIFTED, // Slightly transparent if lifted
        };
        self.with_alpha(alpha)
    }

    pub fn id(&self) -> TileId {
        self.id
    }

    pub fn z(&self) -> f32 {
        self.z
    }

    pub fn alpha(&self) -> f32 {
        self.alpha
    }

    pub fn draw_order(&self) -> impl Ord {
        // Workaround missing Ord for f32
        struct DrawOrder {
            pub z: f32,
            pub id: TileId,
        }

        impl Ord for DrawOrder {
            fn cmp(&self, other: &Self) -> Ordering {
                self.z
                    .partial_cmp(&other.z)
                    .unwrap_or(Ordering::Equal)
                    .then(self.id.cmp(&other.id))
            }
        }

        impl PartialOrd for DrawOrder {
            fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
                Some(self.cmp(other))
            }
        }

        impl PartialEq for DrawOrder {
            fn eq(&self, other: &Self) -> bool {
                self.z == other.z && self.id == other.id
            }
        }

        impl Eq for DrawOrder {}

        DrawOrder {
            z: self.z,
            id: self.id,
        }
    }

    pub fn ui_id(&self) -> Id {
        self.ui_id
    }

    pub fn rect(&self) -> Rect {
        self.rect
    }

    pub fn offset(&self) -> Vec2 {
        self.offset
    }

    pub fn show<R>(self, ui: &mut Ui, add_contents: impl FnOnce(&mut Ui) -> R) -> InnerResponse<R> {
        self.show_dyn(ui, Box::new(add_contents))
    }

    fn show_dyn<'c, R>(
        self,
        ui: &mut Ui,
        add_contents: Box<dyn FnOnce(&mut Ui) -> R + 'c>,
    ) -> InnerResponse<R> {
        let rect = self.rect.translate(self.offset);
        let response = ui.interact(rect, self.ui_id(), Sense::click_and_drag());
        self.paint(ui);

        let mut content_ui = ui.new_child(
            UiBuilder::default()
                .max_rect(rect.shrink2(vec2(MARGIN_HORIZONTAL, MARGIN_VERTICAL)))
                .layout(Layout::top_down(Align::Center))
                .id_salt(self.ui_id()),
        );
        let inner = add_contents(&mut content_ui);
        InnerResponse::new(inner, response)
    }

    fn paint(&self, ui: &Ui) {
        let rect = self.rect.translate(self.offset);
        if !ui.is_rect_visible(rect.expand(CHEVRON_SIZE)) {
            return;
        }

        // Figure out how big each chevron can be without interfering with the
        // rectangle boundaries or other chevrons
        let calc_chevron_sizes = |locations: &[f32]| {
            (0..locations.len())
                .map(|i| {
                    let pos = locations[i];
                    let left_boundary = if i == 0 {
                        0.
                    } else {
                        0.5 * (locations[i - 1] + locations[i])
                    };
                    let right_boundary = if i == locations.len() - 1 {
                        rect.height()
                    } else {
                        0.5 * (locations[i] + locations[i + 1])
                    };

                    CHEVRON_SIZE
                        .min(pos - left_boundary)
                        .min(right_boundary - pos)
                        .max(0.)
                })
                .collect::<Vec<f32>>()
        };

        let input_sizes = calc_chevron_sizes(&self.inputs);
        let output_sizes = calc_chevron_sizes(&self.outputs);

        // Construct the polygon in CCW order starting with the top left
        let mut vertices = Vec::<Pos2>::new();
        vertices.push(pos2(rect.left(), rect.top()));
        let mut last_loc = *self.inputs.first().unwrap_or(&f32::NAN);
        for (&loc, &size) in self.inputs.iter().zip(input_sizes.iter()) {
            assert!(loc >= last_loc, "Inputs are not sorted");
            if size > 0. {
                vertices.push(pos2(rect.left(), rect.top() + loc - size));
                vertices.push(pos2(rect.left() + size, rect.top() + loc));
                vertices.push(pos2(rect.left(), rect.top() + loc + size));
            }
            last_loc = loc;
        }
        vertices.push(pos2(rect.left(), rect.bottom()));
        vertices.push(pos2(rect.right(), rect.bottom()));
        let mut last_loc = *self.outputs.last().unwrap_or(&f32::NAN);
        for (&loc, &size) in self.outputs.iter().rev().zip(output_sizes.iter().rev()) {
            assert!(loc <= last_loc, "Outputs are not sorted");
            if size > 0. {
                vertices.push(pos2(rect.right(), rect.top() + loc + size));
                vertices.push(pos2(rect.right() + size, rect.top() + loc));
                vertices.push(pos2(rect.right(), rect.top() + loc - size));
            }
            last_loc = loc;
        }
        vertices.push(pos2(rect.right(), rect.top()));

        vertices.dedup_by(|a, b| (a.x - b.x).abs() < EPSILON && (a.y - b.y).abs() < EPSILON);

        // Push vertices of polygon
        let fill = (if self.selected {
            FILL_SELECTED
        } else {
            FILL_DESELECTED
        })
        .linear_multiply(self.alpha);
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
                    // NOTE: this ear clipping method is incomplete; we still need to check that the edge p1-p3
                    // lies fully inside the polygon. See winit_output/mod.rs for a complete implementation
                    found_interior = true;
                    mesh.add_triangle(
                        (indices[ii1]) as u32,
                        (indices[ii2]) as u32,
                        (indices[ii3]) as u32,
                    );
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
            assert!(
                found_interior || !found_exterior,
                "Triangulation failed (malformed polygon?)"
            );
        }

        // Paint fill
        ui.painter().add(mesh);

        // Paint stroke
        let stroke = if self.focused {
            STROKE_FOCUSED
        } else {
            STROKE_BLURRED
        };
        let stroke = Stroke {
            width: stroke.width,
            color: stroke.color.linear_multiply(self.alpha),
        };
        ui.painter().add(Shape::closed_line(vertices, stroke));
    }
}
