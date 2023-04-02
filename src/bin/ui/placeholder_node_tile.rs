use radiance::PlaceholderNodeProps;

const NORMAL_HEIGHT: f32 = 200.;
const NORMAL_WIDTH: f32 = 40.;

pub struct PlaceholderNodeTile {}

impl PlaceholderNodeTile {
    /// Returns a Vec with one entry for each props.input_count
    /// corresponding to the minimum allowable height for that input port.
    /// If there are no input ports, this function should return a 1-element Vec.
    pub fn min_input_heights(_props: &PlaceholderNodeProps) -> Vec<f32> {
        // TODO Simplify this to just be a single f32
        (0..1).map(|_| NORMAL_HEIGHT).collect()
    }

    /// Calculates the width of the tile, given its height.
    pub fn width_for_height(_props: &PlaceholderNodeProps, _height: f32) -> f32 {
        NORMAL_WIDTH
    }
}
