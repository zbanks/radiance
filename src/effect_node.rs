use std::string::String;

pub struct EffectNodeArgs {
    name: String,
}

pub struct EffectNodeProps {
    intensity: f32,
}

pub struct EffectNodeState {
}

pub struct EffectNodePaintState {
}

impl EffectNodeState {
    pub fn new() -> EffectNodeState {
        EffectNodeState {}
    }

    pub fn new_paint_state(&self) -> EffectNodePaintState {
        EffectNodePaintState {}
    }

    pub fn paint(&mut self, props: &EffectNodeProps, paint_state: &mut EffectNodePaintState) {
    }
}
