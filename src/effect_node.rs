use crate::types::{Texture, BlankTextureProvider, NoiseTextureProvider};
use wgpu;
use std::rc::Rc;

type EffectNodeContext = dyn BlankTextureProvider;
type EffectNodeChain = dyn NoiseTextureProvider;

pub struct EffectNode {
    context: Rc<EffectNodeContext>,
}

impl EffectNode {
    pub fn new(context: Rc<EffectNodeContext>) -> EffectNode {
        EffectNode {
            context: context,
        }
    }

    pub fn paint(&self, chain: Rc<EffectNodeChain>, input_textures: Vec<Rc<Texture>>) -> Rc<Texture> {
        self.context.blank_texture()
    }
}
