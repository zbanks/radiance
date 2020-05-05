use crate::graphics::{Fbo, RenderChain};
use crate::video_node::{DetailLevel, IVideoNode, VideoNodeDiscriminants, VideoNodeId};
use serde::Serialize;
use serde_json::Value as JsonValue;

use std::rc::Rc;

#[serde(rename_all = "camelCase")]
#[derive(Serialize)]
pub struct OutputNode {
    id: VideoNodeId,
    node_type: VideoNodeDiscriminants,
    n_inputs: usize,
}

impl OutputNode {
    pub fn new() -> OutputNode {
        OutputNode {
            id: VideoNodeId::new(),
            node_type: VideoNodeDiscriminants::OutputNode,
            n_inputs: 1,
        }
    }
}

impl IVideoNode for OutputNode {
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

    fn state(&self, _level: DetailLevel) -> JsonValue {
        serde_json::to_value(&self).unwrap_or(JsonValue::Null)
    }

    fn flush(&self) -> Option<DetailLevel> {
        None
    }
}
