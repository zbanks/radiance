pub mod glsl {
    pub static EFFECT_HEADER: &'static str = include_str!("../static/glsl/effect_header.glsl");
    pub static PLAIN_FRAGMENT: &'static str = include_str!("../static/glsl/plain_fragment.glsl");
    pub static PLAIN_VERTEX: &'static str = include_str!("../static/glsl/plain_vertex.glsl");
}

pub mod effects {
    pub static TEST: &'static str = include_str!("../static/effects/test.glsl");
    pub static PURPLE: &'static str = include_str!("../static/effects/purple.glsl");
    pub static RESAT: &'static str = include_str!("../static/effects/resat.glsl");
}
