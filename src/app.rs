use crate::err::Result;
use crate::graphics::RenderChain;
use crate::model::Graph;
use crate::video_node::{VideoNode, VideoNodeKind};

use log::*;
use std::collections::HashMap;
use std::rc::Rc;
use wasm_bindgen::JsCast;
use web_sys::WebGlRenderingContext;
use yew::prelude::*;
use yew::services::{RenderService, Task};

pub struct App {
    link: ComponentLink<Self>,
    canvas_ref: NodeRef,
    output_ref: NodeRef,
    node_refs: HashMap<usize, NodeRef>,
    render_loop: Option<Box<dyn Task>>,
    chain: Option<RenderChain>,
    model: Model,
}

struct Model {
    graph: Graph,
    show: Option<usize>,
    drop_target: Option<(usize, usize)>,
}

pub enum Msg {
    Render(f64),
    SetIntensity(usize, f64),
    SetChainSize(i32),
    AddEffectNode(String),
    SetDropTarget(usize, usize),
    Reorder(usize),
    //Raise(usize),
}

impl Component for App {
    type Message = Msg;
    type Properties = ();

    fn create(_: Self::Properties, link: ComponentLink<Self>) -> Self {
        let model = Model::new();
        let mut node_refs = HashMap::new();
        for id in model.graph.ids() {
            node_refs.insert(*id, Default::default());
        }

        App {
            link,
            render_loop: None,
            canvas_ref: Default::default(),
            output_ref: Default::default(),
            node_refs,
            model,
            chain: None,
        }
    }

    fn mounted(&mut self) -> ShouldRender {
        let canvas: web_sys::HtmlCanvasElement = self.canvas_ref.cast().unwrap();
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
                let chain = self.chain.as_mut().unwrap();

                // Force the canvas to be the same size as the viewport
                let canvas = self
                    .canvas_ref
                    .cast::<web_sys::HtmlCanvasElement>()
                    .unwrap();
                let canvas_width = canvas.client_width() as u32;
                let canvas_height = canvas.client_height() as u32;
                if canvas.width() != canvas_width {
                    canvas.set_width(canvas_width);
                }
                if canvas.height() != canvas_height {
                    canvas.set_height(canvas_height);
                }

                // Render the chain to textures
                self.model.render(chain, timestamp);

                // Clear the screen
                chain.clear();

                // Paint the previews for each node
                for (id, node_ref) in self.node_refs.iter() {
                    let element = node_ref.cast::<web_sys::Element>().unwrap();
                    self.model.paint_node(chain, *id, &canvas, &element);
                }

                // Paint the (big) output node
                let element = self.output_ref.cast::<web_sys::Element>().unwrap();
                self.model
                    .paint_node(chain, self.model.show.unwrap(), &canvas, &element);

                self.schedule_next_paint();
                false
            }
            Msg::SetIntensity(id, intensity) => {
                let node = self.model.graph.node_mut(id).unwrap();
                if let VideoNodeKind::Effect {
                    intensity: ref mut node_intensity,
                    ..
                } = node.kind
                {
                    *node_intensity = intensity;
                };
                true
            }
            Msg::SetChainSize(size) => {
                let chain_size = (size, size);
                self.chain = Some(
                    RenderChain::new(Rc::clone(&self.chain.as_ref().unwrap().context), chain_size)
                        .unwrap(),
                );
                false
            }
            Msg::AddEffectNode(ref name) => {
                if let Some(drop_target) = self.model.drop_target {
                    if let Ok(id) = self.model.append_node(name, 0.0) {
                        if id != drop_target.0 {
                            self.model
                                .graph
                                .add_edge_by_ids(id, drop_target.0, drop_target.1)
                                .unwrap();
                            self.node_refs.insert(id, Default::default());
                        }
                    }
                }
                true
            }
            Msg::SetDropTarget(id, input) => {
                self.model.drop_target = Some((id, input));
                false
            }
            Msg::Reorder(id) => {
                if let Some(drop_target) = self.model.drop_target {
                    if id != drop_target.0 {
                        if let Some(_node) = self.model.graph.node(id) {
                            // XXX There is a bug here
                            self.model.graph.disconnect_node(id).unwrap();
                            self.model
                                .graph
                                .add_edge_by_ids(id, drop_target.0, drop_target.1)
                                .unwrap();
                        }
                    }
                }
                true
            }
        }
    }

    fn view(&self) -> Html {
        info!("calling view");
        html! {
            <>
                <canvas ref={self.canvas_ref.clone()} width=10 height=10 />

                <h1>{"Radiance"}</h1>
                <div>
                    {"Shader Resolution"}
                    <input
                        type="range"
                        min=1
                        max=1024
                        value=256
                        oninput={
                            self.link.callback(move |e: InputData| Msg::SetChainSize(e.value.parse().unwrap_or(256)))
                        } />
                </div>
                <div>
                    {"Add shader"}
                    <input
                        type="text"
                        oninput=self.link.callback(move |e: InputData| Msg::AddEffectNode(e.value))
                    />
                </div>
                <div class={"output"} ref={self.output_ref.clone()} />
                <div class={"node-list"}>
                    { self.model.graph.root_nodes().iter().map(|n| self.view_node(*n)).collect::<Html>() }
                </div>
            </>
        }
    }
}

impl App {
    fn schedule_next_paint(&mut self) {
        let render_frame = self.link.callback(Msg::Render);
        // TODO: Use requestPostAnimationFrame instead of requestAnimationFrame
        let handle = RenderService::new().request_animation_frame(render_frame);

        // A reference to the new handle must be retained for the next render to run.
        self.render_loop = Some(Box::new(handle));
    }

    fn view_node(&self, node: &VideoNode) -> Html {
        html! {
            <div class={"node-container"}>
                <div class={"node-inputs"}>
                    {
                        self.model.graph.node_inputs(&node).iter().map(|child| html! {
                            <div class={"node-input-row"}>
                                { child.map_or(Default::default(), |c| self.view_node(c)) }
                            </div>
                        }).collect::<Html>()
                    }
                </div>
                <div class={"node"}>
                    <div class={"node-preview"} ref={self.node_refs.get(&node.id).map_or(Default::default(), |x| x.clone())} />
                    {
                        match node.kind {
                            VideoNodeKind::Effect{intensity, ..} => html! {
                                <input
                                    oninput={
                                        let id = node.id;
                                        let old_intensity = intensity;
                                        self.link.callback(move |e: InputData| Msg::SetIntensity(id, e.value.parse().unwrap_or(old_intensity)))
                                    }
                                    value=intensity
                                    type="range"
                                    min=0.
                                    max=1.
                                    step=0.01
                                />
                            },
                            VideoNodeKind::Output => html! {},
                        }
                    }
                    {
                        (0..node.n_inputs).map(|input|
                            html! {
                                <input
                                    type="radio"
                                    checked={ Some((node.id, input)) == self.model.drop_target }
                                    name="drop-target"
                                    oninput={
                                        let id = node.id;
                                        self.link.callback(move |_| Msg::SetDropTarget(id, input))
                                    }
                                />
                            }
                        ).collect::<Html>()
                    }
                    <button
                        onclick={
                            let id = node.id;
                            self.link.callback(move |_| Msg::Reorder(id))
                        }
                    >{"Move"}</button>
                    { &node.name }
                </div>
            </div>
        }
    }
}

impl Model {
    fn new() -> Model {
        let mut model = Model {
            graph: Graph::new(),
            show: None,
            drop_target: None,
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
        Ok(id)
    }

    fn render(&mut self, chain: &mut RenderChain, time: f64) {
        for node in &mut self.graph.nodes_mut() {
            node.set_time(time / 1e3);
        }
        chain.ensure_node_artists(self.graph.nodes_mut()).unwrap();

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
    }

    fn paint_node(
        &mut self,
        chain: &RenderChain,
        id: usize,
        canvas_ref: &web_sys::Element,
        node_ref: &web_sys::Element,
    ) {
        // This assumes that the canvas has style: "position: fixed; left: 0; right: 0;"
        let node = self.graph.node(id).unwrap();

        let canvas_size = (
            chain.context.drawing_buffer_width(),
            chain.context.drawing_buffer_height(),
        );
        let canvas_rect = canvas_ref.get_bounding_client_rect();
        let node_rect = node_ref.get_bounding_client_rect();

        let x_ratio = canvas_rect.width() / canvas_size.0 as f64;
        let y_ratio = canvas_rect.height() / canvas_size.1 as f64;
        let left = (node_rect.left() / x_ratio).ceil();
        let right = (node_rect.right() / x_ratio).floor();
        let top = (node_rect.top() / y_ratio).ceil();
        let bottom = (node_rect.bottom() / y_ratio).floor();

        chain.context.viewport(
            left as i32,
            canvas_size.1 - bottom as i32,
            (right - left) as i32,
            (bottom - top) as i32,
        );
        chain.paint(node).unwrap();
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
