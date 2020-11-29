use crate::types::{Texture, BlankTextureProvider, NoiseTextureProvider};
use wgpu;
use std::rc::Rc;

type EffectNodeContext = dyn BlankTextureProvider;
type EffectNodeChain = dyn NoiseTextureProvider;

pub struct EffectNode<'a> {
    context: &'a EffectNodeContext,
}

impl<'a> EffectNode<'a> {
    pub fn new(context: &EffectNodeContext) -> EffectNode {
        EffectNode {
            context: context,
        }
    }

    pub fn paint(&self, chain: &EffectNodeChain, input_textures: Vec<Rc<Texture>>) -> Rc<Texture> {
        self.context.blank_texture()
    }
}
