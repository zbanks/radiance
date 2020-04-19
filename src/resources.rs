pub mod glsl {
    pub static EFFECT_HEADER: &str = include_str!("../static/glsl/effect_header.glsl");
    pub static PLAIN_FRAGMENT: &str = include_str!("../static/glsl/plain_fragment.glsl");
    pub static PLAIN_VERTEX: &str = include_str!("../static/glsl/plain_vertex.glsl");
}

pub mod effects {
    pub static TEST: &str = include_str!("../static/effects/test.glsl");
    pub static PURPLE: &str = include_str!("../static/effects/purple.glsl");
    pub static RESAT: &str = include_str!("../static/effects/resat.glsl");
}
