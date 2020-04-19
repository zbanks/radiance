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
    model: Option<RenderChain>,
    node_ref: NodeRef,
    render_loop: Option<Box<dyn Task>>,
    nodes: HashMap<usize, VideoNode>,
    graph: Vec<usize>,
    show: Option<usize>
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
        info!("App started");
        App {
            link,
            model: None,
            node_ref: Default::default(),
            render_loop: None,
            nodes: Default::default(),
            graph: Default::default(),
            show: None,
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
        self.model = Some(RenderChain::new(Rc::clone(&context)).unwrap());

        self.append_node("purple").unwrap();
        self.append_node("test").unwrap();
        self.append_node("resat").unwrap();

        self.schedule_next_render();

        true
    }

    fn update(&mut self, msg: Self::Message) -> ShouldRender {
        match msg {
            Msg::Render(timestamp) => {
                self.render_gl(timestamp);
                false
            }
            Msg::SetIntensity(id, intensity) => {
                self.nodes.get_mut(&id).unwrap().set_intensity(intensity);
                true
            }
            Msg::Raise(id) => {
                self.graph.retain(|x| *x != id);
                self.graph.push(id);
                true
            }
            Msg::Show(id) => {
                self.show = Some(id);
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
                    { self.graph.iter().map(|n| self.view_node(*n)).collect::<Html>() }
                </div>
            </div>
        }
    }
}

impl App {
    fn render_gl(&mut self, time: f64) {
        for node in &mut self.nodes.values_mut() {
            node.set_time(time / 1e3);
        }
        let mut nodes = Vec::new();
        for id in &self.graph {
            nodes.push(self.nodes.get(id).unwrap());
        }
        let nodes = nodes;
        //let nodes = self.graph.iter().map(move |i| self.nodes.get(i).unwrap()).collect::<Vec<_>>();
        let model = self.model.as_mut().unwrap();
        model.paint(&nodes).unwrap();

        if let Some(id) = self.show {
            let node = self.nodes.get(&id).unwrap();
            model.render(&node).unwrap();
        }

        self.schedule_next_render();
    }

    fn schedule_next_render(&mut self) {
        let render_frame = self.link.callback(Msg::Render);
        let handle = RenderService::new().request_animation_frame(render_frame);

        // A reference to the new handle must be retained for the next render to run.
        self.render_loop = Some(Box::new(handle));
    }

    fn append_node(&mut self, name: &str) -> Result<()> {
        let node = VideoNode::effect(name)?;
        let id = node.id;
        self.nodes.insert(id, node);
        self.graph.push(id);
        Ok(())
    }

    fn view_node(&self, id: usize) -> Html {
        let node = self.nodes.get(&id).unwrap();
        html! {
            <div>
                { &node.name } {": "}
                <input
                    oninput={
                        let id = node.id;
                        let old_intensity = node.intensity;
                        self.link.callback(move |e: InputData| Msg::SetIntensity(id, e.value.parse().unwrap_or(old_intensity)))
                    }
                    value=node.intensity
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
            </div>
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
