use crate::err::Result;
use crate::graphics::{Fbo, RenderChain};

use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::rc::Rc;

mod effect_node;
mod media_node;
mod output_node;
pub use effect_node::EffectNode;
pub use media_node::MediaNode;
pub use output_node::OutputNode;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Serialize, Deserialize)]
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

// TODO: It's gross to have *both* an enum and a trait object (make up your mind)
// This is currently only used by the UI; I think a reasonable goal is to create
// a (new?) trait for UI methods -- but it's unclear what the UI archetecture will be
// It's possible the UI boundary will just be a set of key-value properties?
pub enum VideoNodeKind<'a> {
    Effect(&'a EffectNode),
    Output(&'a OutputNode),
    Media(&'a MediaNode),
}

pub enum VideoNodeKindMut<'a> {
    Effect(&'a mut EffectNode),
    Output(&'a mut OutputNode),
    Media(&'a mut MediaNode),
}

pub trait VideoNode {
    fn id(&self) -> VideoNodeId;
    fn name(&self) -> &str;

    /// Number of input FBOs
    fn n_inputs(&self) -> usize {
        1
    }

    /// Number of internal FBOs needed during rendering
    fn n_buffers(&self) -> usize {
        0
    }

    /// Set up the VideoNode for rendering. `pre_render` takes a mut reference,
    /// whereas `render` only gets an immutable reference. Perform any actions that require
    /// modifying the VideoNode in this method.
    fn pre_render(&mut self, _chain: &RenderChain, _time: f64) {}

    /// Perform the render operation, returning the rendered result
    fn render<'a>(
        &'a self,
        chain: &'a RenderChain,
        input_fbos: &[Option<Rc<Fbo>>],
        buffer_fbos: &mut [Rc<Fbo>],
    ) -> Option<Rc<Fbo>>;

    fn state(&self) -> JsonValue {
        JsonValue::Null
    }

    fn set_state(&mut self, _state: JsonValue) -> Result<()> {
        Ok(())
    }

    // See TODO about VideoNodeKind
    fn downcast(&self) -> Option<VideoNodeKind> {
        None
    }
    fn downcast_mut(&mut self) -> Option<VideoNodeKindMut> {
        None
    }
}
