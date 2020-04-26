use crate::graphics::{Fbo, RenderChain};

use std::rc::Rc;

mod effect_node;
mod output_node;
pub use effect_node::EffectNode;
pub use output_node::OutputNode;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct VideoNodeId(usize);

impl VideoNodeId {
    fn new() -> VideoNodeId {
        let id = unsafe {
            static mut NEXT_ID: usize = 0;
            NEXT_ID += 1;
            NEXT_ID
        };
        VideoNodeId(id)
    }
}

// TODO: It's gross to have *both* an enum and a trait object; make up your mind
pub enum VideoNodeKind<'a> {
    Effect(&'a EffectNode),
    Output(&'a OutputNode),
}

pub enum VideoNodeKindMut<'a> {
    Effect(&'a mut EffectNode),
    Output(&'a mut OutputNode),
}

pub trait VideoNode {
    fn id(&self) -> VideoNodeId;
    fn name(&self) -> &str;
    fn n_inputs(&self) -> usize {
        1
    }
    fn n_buffers(&self) -> usize {
        0
    }

    fn pre_render(&mut self, _chain: &RenderChain, _time: f64) {}

    fn render<'a>(
        &'a self,
        chain: &'a RenderChain,
        input_fbos: &[Option<Rc<Fbo>>],
        buffer_fbos: &mut [Rc<Fbo>],
    ) -> Option<Rc<Fbo>>;

    // See TODO
    fn downcast(&self) -> Option<VideoNodeKind> {
        None
    }
    fn downcast_mut(&mut self) -> Option<VideoNodeKindMut> {
        None
    }
}
