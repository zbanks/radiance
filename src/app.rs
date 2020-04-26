use crate::err::Result;
use crate::graphics::RenderChain;
use crate::model::Graph;
use crate::video_node::{
    EffectNode, MediaNode, OutputNode, VideoNode, VideoNodeId, VideoNodeKind, VideoNodeKindMut,
};

use log::*;
use std::collections::HashMap;
use std::rc::Rc;
use wasm_bindgen::closure::Closure;
use wasm_bindgen::{JsCast, JsValue};
use web_sys::WebGl2RenderingContext;
use yew::prelude::*;
use yew::services::{RenderService, Task};

pub struct App {
    link: ComponentLink<Self>,
    canvas_ref: NodeRef,
    output_ref: NodeRef,
    node_refs: HashMap<VideoNodeId, NodeRef>,
    video_ref: NodeRef,
    video_promise: Option<Box<Closure<dyn FnMut(JsValue)>>>,
    render_loop: Option<Box<dyn Task>>,
    chain: Option<RenderChain>,
    model: Model,
}

struct Model {
    graph: Graph,
    show: Option<VideoNodeId>,
    drop_target: Option<(VideoNodeId, usize)>,
}

pub enum Msg {
    Render(f64),
    SetIntensity(VideoNodeId, f64),
    SetChainSize(i32),
    AddEffectNode(String),
    SetDropTarget(VideoNodeId, usize),
    Reorder(VideoNodeId),
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
            video_ref: Default::default(),
            video_promise: None,
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
                .get_context("webgl2")
                .expect("WebGL2 not supported")
                .unwrap()
                .dyn_into::<WebGl2RenderingContext>()
                .unwrap(),
        );
        let chain_size = (256, 256);
        self.chain = Some(RenderChain::new(Rc::clone(&context), chain_size).unwrap());

        {
            let video: web_sys::HtmlVideoElement = self.video_ref.cast().unwrap();
            video.set_autoplay(true);

            self.video_promise = Some(Box::new(Closure::once(move |result: JsValue| {
                info!("MediaStream: {:?}", result);
                let media: web_sys::MediaStream = result.dyn_into().unwrap();
                video.set_src_object(Some(&media));
            })));
        }

        {
            let video: web_sys::HtmlVideoElement = self.video_ref.cast().unwrap();
            let node = MediaNode::new(video);
            let node_id = node.id();
            self.node_refs.insert(node_id, Default::default());
            self.model.graph.add_videonode(node);
            let dt = self.model.drop_target.unwrap();
            self.model
                .graph
                .add_edge_by_ids(node_id, dt.0, dt.1)
                .unwrap();
        }

        let mut constraints = web_sys::MediaStreamConstraints::new();
        constraints.audio(&JsValue::FALSE).video(&JsValue::TRUE);
        web_sys::window()
            .unwrap()
            .navigator()
            .media_devices()
            .unwrap()
            .get_user_media_with_constraints(&constraints)
            .unwrap()
            .then(&*self.video_promise.as_ref().unwrap());

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
                match self.model.graph.node_mut(id).and_then(|n| n.downcast_mut()) {
                    Some(VideoNodeKindMut::Effect(node)) => node.set_intensity(intensity),
                    _ => (),
                }
                true
            }
            Msg::SetChainSize(size) => {
                let chain_size = (size, size);
                self.chain.as_mut().unwrap().resize(chain_size);
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

                <video
                    ref={self.video_ref.clone()}
                    width=256
                    height=256
                />
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

    fn view_node(&self, node: &dyn VideoNode) -> Html {
        html! {
            <div class={"node-container"}>
                <div class={"node-inputs"}>
                    {
                        self.model.graph.node_inputs(node).iter().map(|child| html! {
                            <div class={"node-input-row"}>
                                { child.map_or(Default::default(), |c| self.view_node(c)) }
                            </div>
                        }).collect::<Html>()
                    }
                </div>
                <div class={"node"}>
                    <div class={"node-preview"} ref={self.node_refs.get(&node.id()).map_or(Default::default(), |x| x.clone())} />
                    {
                        match node.downcast() {
                            Some(VideoNodeKind::Effect(node)) => html! {
                                <input
                                    oninput={
                                        let id = node.id();
                                        let old_intensity = node.intensity();
                                        self.link.callback(move |e: InputData| Msg::SetIntensity(id, e.value.parse().unwrap_or(old_intensity)))
                                    }
                                    value=node.intensity()
                                    type="range"
                                    min=0.
                                    max=1.
                                    step=0.01
                                />
                            },
                            _ => html! {}
                        }
                    }
                    {
                        (0..node.n_inputs()).map(|input|
                            html! {
                                <input
                                    type="radio"
                                    checked={ Some((node.id(), input)) == self.model.drop_target }
                                    name="drop-target"
                                    oninput={
                                        let id = node.id();
                                        self.link.callback(move |_| Msg::SetDropTarget(id, input))
                                    }
                                />
                            }
                        ).collect::<Html>()
                    }
                    <button
                        onclick={
                            let id = node.id();
                            self.link.callback(move |_| Msg::Reorder(id))
                        }
                    >{"Move"}</button>
                    { node.name() }
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
        ids.push(self.append_node("composite", 0.9)?);

        for (a, b) in ids.iter().zip(ids.iter().skip(1)) {
            self.graph.add_edge_by_ids(*a, *b, 0)?;
        }

        self.show = ids.last().copied();

        let id = self.append_node("test", 0.7)?;
        self.graph.add_edge_by_ids(id, *ids.last().unwrap(), 1)?;
        self.drop_target = Some((id, 0));

        let output_node = OutputNode::new();
        let output_id = output_node.id();
        self.graph.add_videonode(output_node);
        self.graph
            .add_edge_by_ids(*ids.last().unwrap(), output_id, 0)?;

        self.show = Some(output_id);
        info!("State: {}", self.graph.state().to_string());

        Ok(())
    }

    /// This is a temporary utility function that will get refactored
    fn append_node(&mut self, name: &str, value: f64) -> Result<VideoNodeId> {
        let mut node = EffectNode::new(name)?;
        node.set_intensity(value);

        let id = node.id();
        self.graph.add_videonode(node);
        Ok(id)
    }

    fn render(&mut self, chain: &mut RenderChain, time: f64) {
        chain.pre_render(self.graph.nodes_mut(), time);

        chain.context.viewport(0, 0, chain.size.0, chain.size.1);
        for node in self.graph.toposort() {
            let fbos = self
                .graph
                .node_inputs(node)
                .iter()
                .map(|n| n.and_then(|node| chain.node_fbo(node)))
                .collect::<Vec<_>>();
            chain.render_node(node, &fbos);
        }
    }

    fn paint_node(
        &mut self,
        chain: &RenderChain,
        id: VideoNodeId,
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
        let left = ((node_rect.left() - canvas_rect.left()) / x_ratio).ceil();
        let right = ((node_rect.right() - canvas_rect.left()) / x_ratio).floor();
        let top = ((node_rect.top() - canvas_rect.top()) / y_ratio).ceil();
        let bottom = ((node_rect.bottom() - canvas_rect.top()) / y_ratio).floor();

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
