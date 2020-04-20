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
use crate::video_node::VideoNode;
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
    nodes: HashMap<usize, VideoNode>,
    graph: Vec<usize>,
    show: Option<usize>,
}

pub enum Msg {
    Render(f64),
    SetIntensity(usize, f64),
    Raise(usize),
    Show(usize),
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
        self.chain = Some(RenderChain::new(Rc::clone(&context)).unwrap());

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
                self.model.nodes.get_mut(&id).unwrap().intensity = intensity;
                true
            }
            Msg::Raise(id) => {
                self.model.graph.retain(|x| *x != id);
                self.model.graph.push(id);
                true
            }
            Msg::Show(id) => {
                self.model.show = Some(id);
                false
            }
        }
    }

    fn view(&self) -> Html {
        info!("calling view");
        html! {
            <div class="hello">
                <h1>{"Radiance"}</h1>
                <canvas ref={self.node_ref.clone()} width=512 height=512 />
                <div>
                    { self.model.graph.iter().map(|n| self.view_node(*n)).collect::<Html>() }
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

    fn view_node(&self, id: usize) -> Html {
        let node = self.model.nodes.get(&id).unwrap();
        const M: f64 = 1000.;
        html! {
            <div>
                <input
                    oninput={
                        let id = node.id;
                        let old_intensity = node.intensity;
                        self.link.callback(move |e: InputData| Msg::SetIntensity(id, e.value.parse().map_or(old_intensity, |x: f64| x / M)))
                    }
                    value=node.intensity * M
                    type="range"
                    min=0
                    max=M
                />
                <button
                    onclick={
                        let id = node.id;
                        self.link.callback(move |_| Msg::Raise(id))
                    }
                >{"Raise"}</button>
                <button
                    onclick={
                        let id = node.id;
                        self.link.callback(move |_| Msg::Show(id))
                    }
                >{"Show"}</button>

                { &node.name }
            </div>
        }
    }
}

impl Model {
    fn new() -> Model {
        let mut model = Model {
            nodes: Default::default(),
            graph: Default::default(),
            show: None,
        };

        model.append_node("oscope", 1.0).unwrap();
        model.append_node("spin", 0.2).unwrap();
        model.append_node("zoomin", 0.3).unwrap();
        model.append_node("rjump", 0.9).unwrap();
        model.append_node("lpf", 0.3).unwrap();
        model.append_node("tunnel", 0.3).unwrap();
        model.append_node("melt", 0.4).unwrap();
        model.show = model.graph.last().copied();

        model
    }

    fn append_node(&mut self, name: &str, intensity: f64) -> Result<()> {
        let mut node = VideoNode::effect(name)?;
        node.intensity = intensity;
        let id = node.id;
        self.nodes.insert(id, node);
        self.graph.push(id);
        Ok(())
    }

    fn paint(&mut self, chain: &mut RenderChain, time: f64) {
        for node in &mut self.nodes.values_mut() {
            node.set_time(time / 1e3);
        }
        chain.ensure_node_artists(self.nodes.values_mut()).unwrap();

        let mut last_id = None;
        for id in self.graph.iter() {
            let node = self.nodes.get(id).unwrap();
            let artist = chain.node_artist(node).unwrap();
            let last_fbo = last_id
                .and_then(|id| self.nodes.get(id))
                .and_then(|node| chain.node_artist(node).ok())
                .and_then(|artist| artist.fbo());
            artist.render(chain, node, &[last_fbo]);
            last_id = Some(id);
        }

        if let Some(id) = self.show {
            let node = self.nodes.get(&id).unwrap();
            chain.paint(&node).unwrap();
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
