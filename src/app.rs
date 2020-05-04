use crate::audio::Audio;
use crate::err::Result;
use crate::graphics::RenderChain;
use crate::model::Model;
use crate::video_node::{DetailLevel, IVideoNode, VideoNodeId};

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

    graph_changed: Option<js_sys::Function>,
    node_changed: HashMap<VideoNodeId, js_sys::Function>,
}

#[allow(clippy::suspicious_else_formatting)]
#[wasm_bindgen]
impl Context {
    #[wasm_bindgen(constructor)]
    pub fn new(canvas: JsValue, size: i32) -> Option<Context> {
        let canvas_el = canvas.dyn_into::<HtmlCanvasElement>().ok()?;
        let context = Rc::new(
            canvas_el
                .get_context("webgl")
                .expect("WebGL not supported")
                .unwrap()
                .dyn_into::<WebGlRenderingContext>()
                .unwrap(),
        );
        let chain_size = (size, size);
        let chain = RenderChain::new(context, chain_size).unwrap();
        let audio = Audio::new().ok()?;
        Some(Context {
            model: Model::new(),
            audio,
            canvas_el,
            chain,
            graph_changed: Default::default(),
            node_changed: Default::default(),
        })
    }

    //
    // Render functions
    //

    pub fn render(&mut self, time: f64) -> std::result::Result<(), JsValue> {
        //web_sys::console::time_with_label("render");
        //web_sys::console::time_end_with_label("render");
        self.render_internal(time).map_err(|e| e.into())
    }

    fn render_internal(&mut self, time: f64) -> Result<()> {
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

        self.chain.set_audio(self.audio.analyze());

        // Perform pre-render operations that may modify each node
        self.chain.pre_render(self.model.nodes_mut(), time);

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
        Ok(())
    }

    #[wasm_bindgen(js_name=paintNode)]
    pub fn paint_node(
        &mut self,
        id: JsValue,
        node_el: JsValue,
    ) -> std::result::Result<(), JsValue> {
        let node_ref = node_el.dyn_into::<HtmlElement>()?;
        let id: VideoNodeId = id
            .into_serde()
            .map_err(|_| JsValue::from_str("Invalid id, expected Number"))?;
        let node = self.model.node(id).ok_or("Invalid node id")?;

        self.set_canvas_rect(node_ref).map_err(|e| e.to_string())?;
        self.chain.paint(node).unwrap();
        Ok(())
    }

    #[wasm_bindgen(js_name=clearElement)]
    pub fn clear_element(&mut self, element: JsValue) -> std::result::Result<(), JsValue> {
        element
            .dyn_into::<HtmlElement>()
            .and_then(|el| self.set_canvas_rect(el).map_err(|e| e.to_string().into()))?;
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

    pub fn state(&self) -> JsValue {
        JsValue::from_serde(&self.model.state()).unwrap()
    }

    /// Deprecated
    pub fn set_state(&mut self, state: JsValue) -> std::result::Result<(), JsValue> {
        let v: serde_json::Value = state.into_serde().unwrap();
        self.model.set_state(v).unwrap();
        self.flush()?;
        Ok(())
    }

    #[wasm_bindgen(js_name=addEdge)]
    pub fn add_edge(
        &mut self,
        from_vertex: JsValue,
        to_vertex: JsValue,
        to_input: usize,
    ) -> std::result::Result<(), JsValue> {
        let from_id = from_vertex.into_serde().map_err(|e| e.to_string())?;
        let to_id = to_vertex.into_serde().map_err(|e| e.to_string())?;
        self.model
            .add_edge(from_id, to_id, to_input)
            .map_err(|e| e.to_string().into())
    }

    #[wasm_bindgen(js_name=removeEdge)]
    pub fn remove_edge(
        &mut self,
        from_vertex: JsValue,
        to_vertex: JsValue,
        to_input: usize,
    ) -> std::result::Result<(), JsValue> {
        let from_id = from_vertex.into_serde().map_err(|e| e.to_string())?;
        let to_id = to_vertex.into_serde().map_err(|e| e.to_string())?;
        self.model
            .remove_edge(from_id, to_id, to_input)
            .map_err(|e| e.to_string().into())
    }

    pub fn clear(&mut self) {
        self.model.clear();
    }

    //
    // Callbacks & Flushing
    //

    pub fn flush(&mut self) -> std::result::Result<bool, JsValue> {
        let dirt = self.model.flush();
        if dirt.graph {
            if let Some(callback) = &self.graph_changed {
                let state = self.state();
                callback.call1(&JsValue::NULL, &state)?;
            }
        }
        // Prune callbacks for nodes that no longer exist
        let node_ids: HashSet<VideoNodeId> = self.model.ids().copied().collect();
        self.node_changed.retain(|k, _v| node_ids.contains(k));
        for node_id in &dirt.nodes {
            if let Some(callback) = self.node_changed.get(&node_id) {
                self.model
                    .node(*node_id)
                    .and_then(|node| JsValue::from_serde(&node.state(DetailLevel::Local)).ok())
                    .as_ref()
                    .ok_or_else(|| "unable to get state".into())
                    .and_then(|state| callback.call1(&JsValue::NULL, state))?;
            }
        }
        Ok(dirt.graph || !dirt.nodes.is_empty())
    }

    #[wasm_bindgen(js_name=onGraphChanged)]
    pub fn set_graph_changed(&mut self, callback: js_sys::Function) {
        self.graph_changed = Some(callback);
    }

    #[wasm_bindgen(js_name=onNodeChanged)]
    pub fn set_node_changed(
        &mut self,
        id: JsValue,
        _detail: &str,
        callback: js_sys::Function,
    ) -> std::result::Result<(), JsValue> {
        // TODO: use detail
        let id = id.into_serde::<VideoNodeId>().map_err(|e| e.to_string())?;
        self.node_changed.insert(id, callback);
        Ok(())
    }

    //
    // Node functions
    //

    #[wasm_bindgen(js_name=addNode)]
    pub fn add_node(&mut self, state: JsValue) -> std::result::Result<JsValue, JsValue> {
        // TODO: re-write this to be less disjointed
        let id = state
            .into_serde()
            .map_err(|e| e.into())
            .and_then(|s| self.model.add_node(s))
            .map_err(|e| e.to_string())?;
        Ok(JsValue::from_serde(&serde_json::to_value(&id).unwrap()).unwrap())
    }

    #[wasm_bindgen(js_name=removeNode)]
    pub fn remove_node(&mut self, id: JsValue) -> std::result::Result<(), JsValue> {
        id.into_serde()
            .map_err(|e| e.to_string().into())
            .and_then(|id| self.model.remove_node(id).map_err(|e| e.to_string().into()))
    }

    #[wasm_bindgen(js_name=nodeState)]
    pub fn node_state(&self, id: JsValue, level: JsValue) -> std::result::Result<JsValue, JsValue> {
        let id: VideoNodeId = id.into_serde().map_err(|e| e.to_string())?;
        let level: DetailLevel = level.into_serde().map_err(|e| e.to_string())?;
        let node = self.model.node(id).ok_or("invalid id")?;
        JsValue::from_serde(&node.state(level)).map_err(|_| "unserializable state".into())
    }

    #[wasm_bindgen(js_name=setNodeState)]
    pub fn set_node_state(
        &mut self,
        id: JsValue,
        state: JsValue,
    ) -> std::result::Result<(), JsValue> {
        let id: VideoNodeId = id.into_serde().map_err(|e| e.to_string())?;
        let v: serde_json::Value = state.into_serde().map_err(|e| e.to_string())?;
        self.model
            .node_mut(id)
            .ok_or("invalid id")?
            .set_state(v)
            .map_err(|e| e.to_string().into())
    }
}
