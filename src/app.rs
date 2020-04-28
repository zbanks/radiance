use crate::err::Result;
use crate::graphics::RenderChain;
use crate::model::Graph;
use crate::video_node::{
    EffectNode, MediaNode, OutputNode, VideoNode, VideoNodeId, VideoNodeKind, VideoNodeKindMut,
};

use std::rc::Rc;
use log::*;
use wasm_bindgen::prelude::*;
use wasm_bindgen::JsCast;
use web_sys::{WebGl2RenderingContext, HtmlElement, HtmlCanvasElement};

#[wasm_bindgen]
pub struct Model {
    container_el: HtmlElement,
    canvas_el: HtmlCanvasElement,
    graph: Graph,
    chain: RenderChain,
}

#[wasm_bindgen]
impl Model {
    #[wasm_bindgen(constructor)]
    pub fn new(container: JsValue, canvas: JsValue) -> Option<Model> {
        let container_el = container.dyn_into::<HtmlElement>().ok()?;
        let canvas_el = canvas.dyn_into::<HtmlCanvasElement>().ok()?;
        let context = Rc::new(
            canvas_el
                .get_context("webgl2")
                .expect("WebGL2 not supported")
                .unwrap()
                .dyn_into::<WebGl2RenderingContext>()
                .unwrap(),
        );
        let chain_size = (256, 256);
        let chain = RenderChain::new(context, chain_size).unwrap();
        Some(Model {
            graph: Graph::new(),
            container_el,
            canvas_el,
            chain,
        })
    }

    /// This is a temporary utility function that will get refactored
    #[wasm_bindgen]
    pub fn append_node(&mut self, name: &str, value: f64) -> JsValue {
        let mut node = EffectNode::new(name).ok().unwrap();
        node.set_intensity(value);

        let id = node.id();
        self.graph.add_videonode(node);

        JsValue::from_serde(&serde_json::to_value(id).unwrap()).unwrap()
    }

    #[wasm_bindgen]
    pub fn render(&mut self, time: f64) {
        self.chain.pre_render(self.graph.nodes_mut(), time);

        self.chain.context.viewport(0, 0, self.chain.size.0, self.chain.size.1);
        for node in self.graph.toposort() {
            let fbos = self
                .graph
                .node_inputs(node)
                .iter()
                .map(|n| n.and_then(|node| self.chain.node_fbo(node)))
                .collect::<Vec<_>>();
            self.chain.render_node(node, &fbos);
        }
    }

    #[wasm_bindgen]
    pub fn paint_node(
        &mut self,
        id: VideoNodeId,
        node_el: JsValue,
    ) {
        let node_ref = node_el.dyn_into::<HtmlElement>().unwrap();
        // This assumes that the canvas has style: "position: fixed; left: 0; right: 0;"
        let node = self.graph.node(id).unwrap();

        let canvas_size = (
            self.chain.context.drawing_buffer_width(),
            self.chain.context.drawing_buffer_height(),
        );
        let canvas_rect = self.canvas_el.get_bounding_client_rect();
        let node_rect = node_ref.get_bounding_client_rect();

        let x_ratio = canvas_rect.width() / canvas_size.0 as f64;
        let y_ratio = canvas_rect.height() / canvas_size.1 as f64;
        let left = ((node_rect.left() - canvas_rect.left()) / x_ratio).ceil();
        let right = ((node_rect.right() - canvas_rect.left()) / x_ratio).floor();
        let top = ((node_rect.top() - canvas_rect.top()) / y_ratio).ceil();
        let bottom = ((node_rect.bottom() - canvas_rect.top()) / y_ratio).floor();

        self.chain.context.viewport(
            left as i32,
            canvas_size.1 - bottom as i32,
            (right - left) as i32,
            (bottom - top) as i32,
        );
        self.chain.paint(node).unwrap();
    }

    #[wasm_bindgen]
    pub fn state(&self) -> JsValue {
        JsValue::from_serde(&self.graph.state()).unwrap()
    }

    #[wasm_bindgen]
    pub fn set_state(&mut self, state: JsValue) {
        info!("raw: {:?}", state);
        let v: serde_json::Value = state.into_serde().unwrap();
        self.graph.set_state(v).unwrap();
    }
}
