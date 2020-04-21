use log::*;
//use serde_derive::{Deserialize, Serialize};
//use strum::IntoEnumIterator;
//use strum_macros::{EnumIter, ToString};
//use yew::format::Json;
//use yew::services::storage::{Area, StorageService};
use yew::prelude::*;
use yew::services::{RenderService, Task};

use crate::err::Result;
use crate::graphics::RenderChain;
use crate::video_node::{VideoNode, VideoNodeKind};
use petgraph::graphmap::DiGraphMap;
use std::collections::HashMap;
use std::rc::Rc;
use wasm_bindgen::JsCast;
use web_sys::WebGlRenderingContext;

pub struct App {
    link: ComponentLink<Self>,
    node_ref: NodeRef,
    render_loop: Option<Box<dyn Task>>,
    chain: Option<RenderChain>,
    model: Model,
}

struct Model {
    graph: Graph,
    show: Option<usize>,
}

pub enum Msg {
    Render(f64),
    SetIntensity(usize, f64),
    Raise(usize),
}

impl Component for App {
    type Message = Msg;
    type Properties = ();

    fn create(_: Self::Properties, link: ComponentLink<Self>) -> Self {
        App {
            link,
            node_ref: Default::default(),
            render_loop: None,
            chain: None,
            model: Model::new(),
        }
    }

    fn mounted(&mut self) -> ShouldRender {
        let canvas: web_sys::HtmlCanvasElement = self.node_ref.cast().unwrap();
        let context = Rc::new(
            canvas
                .get_context("webgl")
                .expect("WebGL not supported")
                .unwrap()
                .dyn_into::<WebGlRenderingContext>()
                .unwrap(),
        );
        let chain_size = (256, 256);
        self.chain = Some(RenderChain::new(Rc::clone(&context), chain_size).unwrap());

        self.schedule_next_paint();
        true
    }

    fn update(&mut self, msg: Self::Message) -> ShouldRender {
        match msg {
            Msg::Render(timestamp) => {
                self.model.paint(self.chain.as_mut().unwrap(), timestamp);
                self.schedule_next_paint();
                false
            }
            Msg::SetIntensity(id, intensity) => {
                let node = self.model.graph.nodes.get_mut(&id).unwrap();
                if let VideoNodeKind::Effect {
                    intensity: ref mut node_intensity,
                    ..
                } = node.kind
                {
                    *node_intensity = intensity;
                };
                //self.model.graph.nodes.get_mut(&id).unwrap().intensity = intensity;
                true
            }
            Msg::Raise(id) => {
                if let Some(show_id) = self.model.show {
                    if show_id != id {
                        self.model.graph.disconnect_node(id).unwrap();
                        self.model.graph.add_edge_by_ids(show_id, id, 0).unwrap();
                        //self.model.show = Some(id);
                    }
                }
                true
            }
        }
    }

    fn view(&self) -> Html {
        info!("calling view");
        html! {
            <div class="hello">
                <h1>{"Radiance"}</h1>
                <canvas ref={self.node_ref.clone()} width=512 height=512 />
                <div class={"node-list"}>
                    { self.model.graph.toposort().iter().map(|n| self.view_node(*n)).collect::<Html>() }
                </div>
            </div>
        }
    }
}

impl App {
    fn schedule_next_paint(&mut self) {
        let render_frame = self.link.callback(Msg::Render);
        let handle = RenderService::new().request_animation_frame(render_frame);

        // A reference to the new handle must be retained for the next render to run.
        self.render_loop = Some(Box::new(handle));
    }

    fn view_node(&self, node: &VideoNode) -> Html {
        const M: f64 = 1000.;
        html! {
            <div class={"node"}>
                {
                    match node.kind {
                        VideoNodeKind::Effect{intensity, ..} => html! {
                            <input
                                oninput={
                                    let id = node.id;
                                    let old_intensity = intensity;
                                    self.link.callback(move |e: InputData| Msg::SetIntensity(id, e.value.parse().map_or(old_intensity, |x: f64| x / M)))
                                }
                                value=intensity * M
                                type="range"
                                min=0
                                max=M
                            />
                        },
                        VideoNodeKind::Output => html! {},
                    }
                }
                <button
                    onclick={
                        let id = node.id;
                        self.link.callback(move |_| Msg::Raise(id))
                    }
                >{"Raise"}</button>

                { &node.name }
            </div>
        }
    }
}

/// Directed graph abstraction that owns VideoNodes
/// - Enforces that there are no cycles
/// - Each VideoNode can have up to `node.n_inputs` incoming edges,
///     which must all have unique edge weights in [0..node.n_inputs)
struct Graph {
    nodes: HashMap<usize, VideoNode>,
    digraph: DiGraphMap<usize, usize>,
}

impl Graph {
    fn new() -> Graph {
        Graph {
            digraph: DiGraphMap::new(),
            nodes: Default::default(),
        }
    }

    fn add_videonode(&mut self, node: VideoNode) {
        self.digraph.add_node(node.id);
        self.nodes.insert(node.id, node);
    }

    /*
    fn remove_videonode(&mut self, node: &VideoNode) {
        self.digraph.remove_node(node.id);
        self.nodes.remove(&node.id);
    }
    */

    fn add_edge_by_ids(&mut self, src_id: usize, dst_id: usize, input: usize) -> Result<()> {
        // TODO: safety check
        if src_id == dst_id {
            return Err("Adding self edge would cause cycle".into());
        }
        self.digraph.add_edge(src_id, dst_id, input);
        self.assert_no_cycles();
        Ok(())
    }

    fn toposort(&self) -> Vec<&VideoNode> {
        petgraph::algo::toposort(&self.digraph, None)
            .unwrap()
            .iter()
            .map(|id| self.nodes.get(id).unwrap())
            .collect()
    }

    fn node_inputs(&self, node: &VideoNode) -> Vec<Option<&VideoNode>> {
        let mut inputs = Vec::new();
        inputs.resize(node.n_inputs, None);

        for src_id in self
            .digraph
            .neighbors_directed(node.id, petgraph::Direction::Incoming)
        {
            let src_index = *self.digraph.edge_weight(src_id, node.id).unwrap();
            if src_index < node.n_inputs {
                inputs[src_index] = self.nodes.get(&src_id);
            }
        }

        inputs
    }

    fn disconnect_node(&mut self, id: usize) -> Result<()> {
        let node = self.nodes.get(&id).unwrap();
        let inputs = self.node_inputs(node);
        let src_id = inputs.first().unwrap_or(&None).map(|n| n.id);
        let edges_to_remove: Vec<(usize, usize)> = self
            .digraph
            .neighbors_directed(node.id, petgraph::Direction::Outgoing)
            .map(|dst_id| {
                let dst_index = *self.digraph.edge_weight(node.id, dst_id).unwrap();
                (dst_id, dst_index)
            })
            .collect();
        for (dst_id, dst_index) in edges_to_remove {
            self.digraph.remove_edge(node.id, dst_id);
            if let Some(id) = src_id {
                self.digraph.add_edge(id, dst_id, dst_index);
            }
        }
        self.assert_no_cycles();
        Ok(())
    }

    fn assert_no_cycles(&self) {
        petgraph::algo::toposort(&self.digraph, None).unwrap();
    }
}

impl Model {
    fn new() -> Model {
        let mut model = Model {
            graph: Graph::new(),
            show: None,
        };

        model.setup().unwrap();
        model
    }

    /// This is a temporary utility function that will get refactored
    fn setup(&mut self) -> Result<()> {
        let mut ids = vec![];
        ids.push(self.append_node("oscope", 1.0)?);
        ids.push(self.append_node("spin", 0.2)?);
        ids.push(self.append_node("zoomin", 0.3)?);
        ids.push(self.append_node("rjump", 0.9)?);
        ids.push(self.append_node("lpf", 0.3)?);
        ids.push(self.append_node("tunnel", 0.3)?);
        ids.push(self.append_node("melt", 0.4)?);
        ids.push(self.append_node("composite", 0.5)?);

        for (a, b) in ids.iter().zip(ids.iter().skip(1)) {
            self.graph.add_edge_by_ids(*a, *b, 0)?;
        }

        self.show = ids.last().copied();

        let id = self.append_node("test", 0.7)?;
        self.graph.add_edge_by_ids(id, *ids.last().unwrap(), 1)?;

        let output_node = VideoNode::output()?;
        let output_id = output_node.id;
        self.graph.add_videonode(output_node);
        self.graph
            .add_edge_by_ids(*ids.last().unwrap(), output_id, 0)?;

        self.show = Some(output_id);

        Ok(())
    }

    /// This is a temporary utility function that will get refactored
    fn append_node(&mut self, name: &str, value: f64) -> Result<usize> {
        let mut node = VideoNode::effect(name)?;
        if let VideoNodeKind::Effect {
            ref mut intensity, ..
        } = node.kind
        {
            *intensity = value;
        }

        let id = node.id;
        self.graph.add_videonode(node);
        /*
        if let Some(prev_id) = self.show {
            self.graph.add_edge_by_ids(prev_id, id, 0)?;
        }
        self.show = Some(id);
        */
        Ok(id)
    }

    fn paint(&mut self, chain: &mut RenderChain, time: f64) {
        for node in &mut self.graph.nodes.values_mut() {
            node.set_time(time / 1e3);
        }
        chain
            .ensure_node_artists(self.graph.nodes.values_mut())
            .unwrap();

        chain.context.viewport(0, 0, chain.size.0, chain.size.1);
        for node in self.graph.toposort() {
            let artist = chain.node_artist(node).unwrap();
            let fbos = self
                .graph
                .node_inputs(node)
                .iter()
                .map(|n| {
                    n.and_then(|node| chain.node_artist(node).ok())
                        .and_then(|artist| artist.fbo())
                })
                .collect::<Vec<_>>();
            artist.render(chain, node, &fbos);
        }

        let canvas_size = (
            chain.context.drawing_buffer_width(),
            chain.context.drawing_buffer_height(),
        );
        chain.context.clear(WebGlRenderingContext::COLOR_BUFFER_BIT);
        if let Some(id) = self.show {
            chain.context.viewport(0, canvas_size.1 - 300, 300, 300);

            let node = self.graph.nodes.get(&id).unwrap();
            chain.paint(&node).unwrap();
        }

        for (i, node) in self.graph.toposort().iter().enumerate() {
            chain
                .context
                .viewport(310, canvas_size.1 - (50 * i) as i32 - 50, 48, 48);
            chain.paint(node).unwrap();
        }
    }
}

/*
async fn fetch_resource(resource: &str) -> Result<String, JsValue> {
    let mut opts = RequestInit::new();
    opts.method("GET");
    //opts.mode(RequestMode::Cors);
    let request = Request::new_with_str_and_init(resource, &opts)?;
    let window = web_sys::window().unwrap();
    let resp_value = JsFuture::from(window.fetch_with_request(&request)).await?;

    // `resp_value` is a `Response` object.
    assert!(resp_value.is_instance_of::<Response>());
    let resp: Response = resp_value.dyn_into().unwrap();
    let text = JsFuture::from(resp.text()?).await?;
    Ok(text.as_string().unwrap())
}
*/
