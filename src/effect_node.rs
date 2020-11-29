use crate::types::{Texture, BlankTextureProvider, NoiseTextureProvider};
use wgpu;
use std::rc::Rc;

type EffectNodeContext = BlankTextureProvider;
type EffectNodeChain = NoiseTextureProvider;

struct EffectNode {
}

impl EffectNode {
    pub fn new() {
    }

    pub fn paint(context: &EffectNodeContext, chain: &EffectNodeChain, input_textures: Vec<Rc<Texture>>) -> Rc<Texture> {
        context.blank_texture()
    }
}
