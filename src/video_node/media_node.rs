use crate::err::Result;
use crate::graphics::{Fbo, RenderChain};
use crate::video_node::{VideoNode, VideoNodeId, VideoNodeType};

use log::*;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::rc::Rc;
use wasm_bindgen::closure::Closure;
use wasm_bindgen::{JsCast, JsValue};
use web_sys::HtmlVideoElement;

/// TODO: Add support for HtmlImageElement
pub struct MediaNode {
    video: HtmlVideoElement,
    video_promise: Option<Box<Closure<dyn FnMut(JsValue)>>>,

    state: State,
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Serialize, Deserialize)]
struct State {
    #[serde(rename = "uid")]
    id: Option<VideoNodeId>,
    node_type: VideoNodeType,
    n_inputs: usize,
    video_element_id: Option<String>,
}

impl MediaNode {
    pub fn new() -> Result<MediaNode> {
        let video: HtmlVideoElement = web_sys::window()
            .ok_or("cannot get window")?
            .document()
            .ok_or("cannot get document")?
            .create_element("video")?
            .dyn_into()
            .map_err(|_| "cannot get video element")?;
        video.set_autoplay(true);
        let video_clone = video.clone();
        let video_promise = Some(Box::new(Closure::once(move |result: JsValue| {
            info!("MediaStream: {:?}", result);
            let media: web_sys::MediaStream = result.dyn_into().unwrap();
            video_clone.set_src_object(Some(&media));
        })));

        let state = State {
            id: Some(VideoNodeId::new()),
            node_type: VideoNodeType::Media,
            n_inputs: 1,
            video_element_id: None,
        };
        let node = MediaNode {
            video,
            video_promise,
            state,
        };

        let mut constraints = web_sys::MediaStreamConstraints::new();
        constraints.audio(&JsValue::FALSE).video(&JsValue::TRUE);
        web_sys::window()
            .unwrap()
            .navigator()
            .media_devices()
            .unwrap()
            .get_user_media_with_constraints(&constraints)
            .unwrap()
            .then(&*node.video_promise.as_ref().unwrap());

        Ok(node)
    }

    fn media_stream_callback(&self, result: JsValue) {
        let media: web_sys::MediaStream = result.dyn_into().unwrap();
        self.video.set_src_object(Some(&media));
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

        if self.video.ready_state() == 4 {
            buffer_fbos[0]
                .image_video_element(&self.video)
                .ok()
                .map(|_| Rc::clone(&buffer_fbos[0]))
        } else {
            None
        }
    }

    fn state(&self) -> JsonValue {
        serde_json::to_value(&self.state).unwrap_or(JsonValue::Null)
    }
}
