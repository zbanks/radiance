use crate::err::Result;
use crate::graphics::{Fbo, RenderChain};
use crate::video_node::{VideoNode, VideoNodeId, VideoNodeType};

use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::rc::Rc;
use web_sys::HtmlVideoElement;

/// TODO: Add support for HtmlImageElement
pub struct MediaNode {
    video: Option<HtmlVideoElement>,
    state: State,
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Serialize, Deserialize)]
struct State {
    #[serde(rename = "uid")]
    id: Option<VideoNodeId>,
    node_type: VideoNodeType,
    video_element_id: Option<String>,
}

impl MediaNode {
    pub fn new() -> MediaNode {
        let state = State {
            id: Some(VideoNodeId::new()),
            node_type: VideoNodeType::Media,
            video_element_id: None,
        };
        MediaNode { video: None, state }
    }

    pub fn set_video_element_id(&mut self, video_element_id: Option<String>) -> Result<()> {
        if let Some(video_element_id) = video_element_id {
            self.state.video_element_id = Some(video_element_id);
        // self.video = ???
        } else {
            self.state.video_element_id = None;
            self.video = None;
        }
        Ok(())
    }
}

impl VideoNode for MediaNode {
    fn id(&self) -> VideoNodeId {
        self.state.id.unwrap()
    }

    fn name(&self) -> &str {
        "Media"
    }

    fn n_buffers(&self) -> usize {
        1
    }

    fn render<'a>(
        &'a self,
        _chain: &'a RenderChain,
        input_fbos: &[Option<Rc<Fbo>>],
        buffer_fbos: &mut [Rc<Fbo>],
    ) -> Option<Rc<Fbo>> {
        assert!(input_fbos.len() == self.n_inputs());
        assert!(buffer_fbos.len() == self.n_buffers());

        let video = self.video.as_ref()?;
        if video.ready_state() == 4 {
            buffer_fbos[0]
                .image_video_element(video)
                .ok()
                .map(|_| Rc::clone(&buffer_fbos[0]))
        } else {
            None
        }
    }

    fn state(&self) -> JsonValue {
        serde_json::to_value(&self.state).unwrap_or(JsonValue::Null)
    }

    fn set_state(&mut self, raw_state: JsonValue) -> Result<()> {
        let state: State = serde_json::from_value(raw_state)?;
        if self.state.video_element_id != state.video_element_id {
            self.set_video_element_id(state.video_element_id)?;
        }
        Ok(())
    }
}
