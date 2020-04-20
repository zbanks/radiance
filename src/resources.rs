pub mod glsl {
    pub static EFFECT_HEADER: &str = include_str!("../static/glsl/effect_header.glsl");
    pub static PLAIN_FRAGMENT: &str = include_str!("../static/glsl/plain_fragment.glsl");
    pub static PLAIN_VERTEX: &str = include_str!("../static/glsl/plain_vertex.glsl");
}

pub mod effects {
    pub static LPF: &str = include_str!("../static/effects/lpf.glsl");
    pub static MELT: &str = include_str!("../static/effects/melt.glsl");
    pub static OSCOPE: &str = include_str!("../static/effects/oscope.glsl");
    pub static PURPLE: &str = include_str!("../static/effects/purple.glsl");
    pub static RESAT: &str = include_str!("../static/effects/resat.glsl");
    pub static RJUMP: &str = include_str!("../static/effects/rjump.glsl");
    pub static SPIN: &str = include_str!("../static/effects/spin.glsl");
    pub static TEST: &str = include_str!("../static/effects/test.glsl");
    pub static TUNNEL: &str = include_str!("../static/effects/tunnel.glsl");
    pub static ZOOMIN: &str = include_str!("../static/effects/zoomin.glsl");

    pub fn lookup(name: &str) -> Option<&'static str> {
        match name {
            "lpf" => Some(LPF),
            "melt" => Some(MELT),
            "oscope" => Some(OSCOPE),
            "purple" => Some(PURPLE),
            "resat" => Some(RESAT),
            "rjump" => Some(RJUMP),
            "spin" => Some(SPIN),
            "test" => Some(TEST),
            "tunnel" => Some(TUNNEL),
            "zoomin" => Some(ZOOMIN),
            _ => None,
        }
    }
}
