use crate::graphics::{Fbo, RenderChain};
use crate::video_node::{VideoNode, VideoNodeId};

use std::rc::Rc;

pub struct OutputNode {
    id: VideoNodeId,
}

impl OutputNode {
    pub fn new() -> OutputNode {
        OutputNode {
            id: VideoNodeId::new(),
        }
    }
}

impl VideoNode for OutputNode {
    fn id(&self) -> VideoNodeId {
        self.id
    }

    fn name(&self) -> &str {
        "Output"
    }

    fn render<'a>(
        &'a self,
        _chain: &'a RenderChain,
        input_fbos: &[Option<Rc<Fbo>>],
        buffer_fbos: &mut [Rc<Fbo>],
    ) -> Option<Rc<Fbo>> {
        assert!(input_fbos.len() == self.n_inputs());
        assert!(buffer_fbos.len() == self.n_buffers());

        input_fbos.first().unwrap().as_ref().cloned()
    }
}
