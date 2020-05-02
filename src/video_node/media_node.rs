use crate::err::Result;
use crate::graphics::{Fbo, RenderChain};
use crate::video_node::{DetailLevel, VideoNode, VideoNodeId, VideoNodeType};

use log::*;
use serde::Serialize;
use serde_json::Value as JsonValue;
use std::rc::Rc;
use wasm_bindgen::closure::Closure;
use wasm_bindgen::{JsCast, JsValue};
use web_sys::HtmlVideoElement;

/// TODO: Add support for HtmlImageElement
#[serde(rename_all = "camelCase")]
#[derive(Serialize)]
pub struct MediaNode {
    #[serde(rename = "uid")]
    id: VideoNodeId,
    node_type: VideoNodeType,
    n_inputs: usize,

    #[serde(skip)]
    video: HtmlVideoElement,
    #[serde(skip)]
    #[allow(clippy::type_complexity)]
    video_promise: Option<Box<Closure<dyn FnMut(JsValue)>>>,
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

        let node = MediaNode {
            id: VideoNodeId::new(),
            node_type: VideoNodeType::Media,
            n_inputs: 1,
            video,
            video_promise,
        };

        let mut constraints = web_sys::MediaStreamConstraints::new();
        constraints.audio(&JsValue::FALSE).video(&JsValue::TRUE);
        let _ = web_sys::window()
            .unwrap()
            .navigator()
            .media_devices()
            .unwrap()
            .get_user_media_with_constraints(&constraints)
            .unwrap()
            .then(&*node.video_promise.as_ref().unwrap());

        Ok(node)
    }
}

impl VideoNode for MediaNode {
    fn id(&self) -> VideoNodeId {
        self.id
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

    fn state(&self, _level: DetailLevel) -> JsonValue {
        serde_json::to_value(&self).unwrap_or(JsonValue::Null)
    }
}
