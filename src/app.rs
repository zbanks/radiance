use crate::audio::Audio;
use crate::err::{Error, JsResult, Result};
use crate::graphics::RenderChain;
use crate::library::{ContentHash, Library};
use crate::model::Model;
use crate::video_node::{DetailLevel, IVideoNode, VideoNodeId};

use log::*;
use std::cell::RefCell;
use std::collections::{HashMap, HashSet};
use std::rc::Rc;
use wasm_bindgen::prelude::*;
use wasm_bindgen::JsCast;
use web_sys::{HtmlCanvasElement, HtmlElement, WebGlRenderingContext};

#[wasm_bindgen]
pub struct Context {
    canvas_el: HtmlCanvasElement,
    audio: Audio,
    model: Model,
    chain: RenderChain,
    library: Library,

    graph_changed: Option<js_sys::Function>,
    node_changed: RefCell<HashMap<VideoNodeId, (DetailLevel, js_sys::Function)>>,
}

#[allow(clippy::suspicious_else_formatting)]
#[wasm_bindgen]
impl Context {
    #[wasm_bindgen(constructor)]
    pub fn new(canvas: JsValue, size: i32) -> JsResult<Context> {
        info!("Initializing Backend Context");
        let canvas_el = canvas.dyn_into::<HtmlCanvasElement>()?;
        let context = Rc::new(
            canvas_el
                .get_context("webgl")
                .ok()
                .flatten()
                .ok_or_else(|| Error::missing_feature("WebGL"))?
                .dyn_into::<WebGlRenderingContext>()?,
        );
        let chain_size = (size, size);
        let chain = RenderChain::new(context, chain_size)?;
        let audio = Audio::new()?;
        let library = Library::new();
        library.load_all();

        Ok(Context {
            model: Model::new(),
            audio,
            canvas_el,
            chain,
            library,
            graph_changed: Default::default(),
            node_changed: Default::default(),
        })
    }

    //
    // Render functions
    //
    pub fn render(&mut self, time: f64) -> JsResult<()> {
        // Resize the canvas to if it changed size
        // This doesn't need to happen before rendering; just before calling `paint_node()`
        let canvas_width = self.canvas_el.client_width() as u32;
        let canvas_height = self.canvas_el.client_height() as u32;
        if self.canvas_el.width() != canvas_width {
            self.canvas_el.set_width(canvas_width);
        }
        if self.canvas_el.height() != canvas_height {
            self.canvas_el.set_height(canvas_height);
        }

        self.chain.set_audio(self.audio.analyze(time / 1e3));

        // Perform pre-render operations that may modify each node
        self.chain.pre_render(self.model.nodes_mut());
        self.model.check_nodes();

        // Render each node sequentially in topological order
        self.chain
            .set_drawing_rect(0, 0, self.chain.size.0, self.chain.size.1);
        for node in self.model.toposort() {
            let fbos = self
                .model
                .node_inputs(node)
                .iter()
                .map(|n| n.and_then(|node| self.chain.node_fbo(node)))
                .collect::<Vec<_>>();
            //web_sys::console::time_with_label("render_node");
            self.chain.render_node(node, &fbos);
            //web_sys::console::time_end_with_label("render_node");
        }

        // Trigger any node change events
        self.flush_nodes()?;
        Ok(())
    }

    #[wasm_bindgen(js_name=paintNode)]
    pub fn paint_node(&mut self, id: JsValue, node_el: JsValue) -> JsResult<()> {
        let node_ref = node_el.dyn_into::<HtmlElement>()?;
        let id: VideoNodeId = id.into_serde().map_err(Error::serde)?;
        let node = self.model.node(id)?;

        self.set_canvas_rect(node_ref)?;
        self.chain.paint(node)?;
        Ok(())
    }

    #[wasm_bindgen(js_name=clearElement)]
    pub fn clear_element(&mut self, element: JsValue) -> JsResult<()> {
        self.set_canvas_rect(element.dyn_into::<HtmlElement>()?)?;
        self.chain.clear();
        Ok(())
    }

    /// Set the context's viewport & scissor to the bounding box of an HtmlElement
    fn set_canvas_rect(&self, el_ref: HtmlElement) -> Result<()> {
        // This assumes that the canvas has style: "position: fixed; left: 0; right: 0;"
        let canvas_size = (
            self.chain.context.drawing_buffer_width(),
            self.chain.context.drawing_buffer_height(),
        );
        let node_rect = el_ref.get_bounding_client_rect();
        let canvas_rect = self.canvas_el.get_bounding_client_rect();

        let x_ratio = canvas_rect.width() / canvas_size.0 as f64;
        let y_ratio = canvas_rect.height() / canvas_size.1 as f64;
        let left = ((node_rect.left() - canvas_rect.left()) / x_ratio).ceil();
        let right = ((node_rect.right() - canvas_rect.left()) / x_ratio).floor();
        let top = ((node_rect.top() - canvas_rect.top()) / y_ratio).ceil();
        let bottom = ((node_rect.bottom() - canvas_rect.top()) / y_ratio).floor();

        self.chain.set_drawing_rect(
            left as i32,
            canvas_size.1 - bottom as i32,
            (right - left) as i32,
            (bottom - top) as i32,
        );
        Ok(())
    }

    //
    // Graph/Edge Functions
    //

    pub fn state(&self) -> JsResult<JsValue> {
        JsValue::from_serde(&self.model.state())
            .map_err(Error::serde)
            .map_err(|e| e.into())
    }

    #[wasm_bindgen(js_name=addEdge)]
    pub fn add_edge(
        &mut self,
        from_vertex: JsValue,
        to_vertex: JsValue,
        to_input: usize,
    ) -> JsResult<()> {
        let from_id = from_vertex.into_serde().map_err(Error::serde)?;
        let to_id = to_vertex.into_serde().map_err(Error::serde)?;
        self.model
            .add_edge(from_id, to_id, to_input)
            .map_err(|e| e.into())
    }

    #[wasm_bindgen(js_name=removeEdge)]
    pub fn remove_edge(
        &mut self,
        from_vertex: JsValue,
        to_vertex: JsValue,
        to_input: usize,
    ) -> JsResult<()> {
        let from_id = from_vertex.into_serde().map_err(Error::serde)?;
        let to_id = to_vertex.into_serde().map_err(Error::serde)?;
        self.model
            .remove_edge(from_id, to_id, to_input)
            .map_err(|e| e.into())
    }

    pub fn clear(&mut self) -> JsResult<()> {
        self.model.clear();
        Ok(())
    }

    //
    // Callbacks & Flushing
    //

    pub fn flush(&self) -> JsResult<bool> {
        if self.model.flush() {
            if let Some(callback) = &self.graph_changed {
                let state = self.state()?;
                callback.call1(&JsValue::NULL, &state)?;
            }
            Ok(true)
        } else {
            Ok(false)
        }
    }

    fn flush_nodes(&self) -> JsResult<()> {
        let node_ids: HashSet<VideoNodeId> = self.model.ids().collect();
        self.node_changed
            .borrow_mut()
            .retain(|k, _v| node_ids.contains(k));
        for node in self.model.nodes() {
            if let Some(dirty_level) = node.flush() {
                if let Some((level, callback)) = self.node_changed.borrow().get(&node.id()) {
                    if *level >= dirty_level {
                        JsValue::from_serde(&node.state(*level))
                            .as_ref()
                            .map_err(|e| e.to_string().into())
                            .and_then(|state| callback.call1(&JsValue::NULL, state))?;
                    }
                }
            }
        }
        Ok(())
    }

    #[wasm_bindgen(js_name=onGraphChanged)]
    pub fn set_graph_changed(&mut self, callback: js_sys::Function) -> JsResult<()> {
        self.graph_changed = Some(callback);
        Ok(())
    }

    #[wasm_bindgen(js_name=onNodeChanged)]
    pub fn set_node_changed(
        &self,
        id: JsValue,
        detail_level: JsValue,
        callback: js_sys::Function,
    ) -> JsResult<()> {
        let id = id.into_serde::<VideoNodeId>().map_err(Error::serde)?;
        let level: DetailLevel = detail_level.into_serde().map_err(Error::serde)?;
        self.node_changed.borrow_mut().insert(id, (level, callback));
        Ok(())
    }

    //
    // Node functions
    //

    #[wasm_bindgen(js_name=addNode)]
    pub fn add_node(&mut self, state: JsValue) -> JsResult<JsValue> {
        let id = state
            .into_serde()
            .map_err(|e| e.into())
            .and_then(|s| self.model.add_node(s, &self.library))?;

        serde_json::to_value(&id)
            .and_then(|v| JsValue::from_serde(&v))
            .map_err(Error::serde)
            .map_err(|e| e.into())
    }

    #[wasm_bindgen(js_name=removeNode)]
    pub fn remove_node(&mut self, id: JsValue) -> JsResult<()> {
        id.into_serde()
            .map_err(|e| e.into())
            .and_then(|id| self.model.remove_node(id))
            .map_err(|e| e.into())
    }

    #[wasm_bindgen(js_name=nodeState)]
    pub fn node_state(&self, id: JsValue, detail_level: JsValue) -> JsResult<JsValue> {
        let id: VideoNodeId = id.into_serde().map_err(Error::serde)?;
        let level: DetailLevel = detail_level.into_serde().map_err(Error::serde)?;
        let node = self.model.node(id)?;
        JsValue::from_serde(&node.state(level)).map_err(|e| Error::serde(e).into())
    }

    #[wasm_bindgen(js_name=setNodeState)]
    pub fn set_node_state(&mut self, id: JsValue, state: JsValue) -> JsResult<()> {
        let id: VideoNodeId = id.into_serde().map_err(Error::serde)?;
        let v: serde_json::Value = state.into_serde().map_err(Error::serde)?;
        self.model.node_mut(id)?.set_state(v).map_err(|e| e.into())
    }

    #[wasm_bindgen(js_name=library)]
    pub fn library(&self) -> JsResult<JsValue> {
        JsValue::from_serde(&self.library.items()).map_err(|e| Error::serde(e).into())
    }

    #[wasm_bindgen(js_name=libraryContent)]
    pub fn library_content(&self, hash: JsValue) -> JsResult<JsValue> {
        let hash: ContentHash = hash.into_serde().map_err(Error::serde)?;
        self.library
            .content(&hash)
            .map(|x| x.into())
            .ok_or_else(|| format!("Invalid hash: {:?}", hash).into())
    }
}
