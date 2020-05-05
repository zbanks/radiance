use crate::err::Result;
use crate::graphics::{Fbo, RenderChain};

use enum_dispatch::enum_dispatch;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::rc::Rc;
use strum_macros::{EnumDiscriminants, EnumString};
use wasm_bindgen::prelude::*;

mod effect_node;
mod media_node;
mod output_node;
pub use effect_node::EffectNode;
pub use media_node::MediaNode;
pub use output_node::OutputNode;

#[wasm_bindgen]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Serialize, Deserialize)]
pub struct VideoNodeId(usize);

impl VideoNodeId {
    fn new() -> VideoNodeId {
        let id = unsafe {
            static mut NEXT_ID: usize = 100;
            NEXT_ID += 1;
            NEXT_ID
        };
        VideoNodeId(id)
    }
}

#[enum_dispatch]
#[strum_discriminants(derive(EnumString, Serialize, Deserialize))]
#[derive(EnumDiscriminants)]
pub enum VideoNode {
    EffectNode,
    OutputNode,
    MediaNode,
}

#[wasm_bindgen]
#[serde(rename_all = "camelCase")]
#[derive(Debug, Serialize, Deserialize, PartialOrd, Ord, PartialEq, Eq, Copy, Clone)]
pub enum DetailLevel {
    /// Some of the writable state. Used for long-term storage of models, such as in save files.
    Export,
    /// Most or all of the writable state. Used to cut, copy, or duplicate a node within the localized content of a session. Should include things like video timestamp.
    Local,
    /// All of the state, including read-only properties. Used for updating the UI.
    All,
}

#[enum_dispatch(VideoNode)]
pub trait IVideoNode {
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

    fn state(&self, _level: DetailLevel) -> JsonValue {
        JsonValue::Null
    }

    fn set_state(&mut self, _state: JsonValue) -> Result<()> {
        Ok(())
    }

    /// Return the maximum level of detail that has changed, if any, since the last `flush()` call
    fn flush(&self) -> Option<DetailLevel> {
        // A conservative default implementation, which always says everything has changed
        Some(DetailLevel::All)
    }
}

impl VideoNode {
    pub fn from_serde(state: JsonValue) -> Result<VideoNode> {
        let node_type = state.get("nodeType").ok_or("missing 'nodeType'")?;
        let mut node = match serde_json::from_value(node_type.clone())? {
            VideoNodeDiscriminants::EffectNode => VideoNode::EffectNode(EffectNode::new()?),
            VideoNodeDiscriminants::OutputNode => VideoNode::OutputNode(OutputNode::new()),
            VideoNodeDiscriminants::MediaNode => VideoNode::MediaNode(MediaNode::new()?),
        };
        node.set_state(state)?;
        Ok(node)
    }
}
