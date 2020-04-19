use log::*;
//use serde_derive::{Deserialize, Serialize};
//use strum::IntoEnumIterator;
//use strum_macros::{EnumIter, ToString};
//use yew::format::Json;
//use yew::services::storage::{Area, StorageService};
use yew::prelude::*;
use yew::services::{RenderService, Task};

use crate::graphics::RenderChain;
use crate::resources;
use crate::video_node::VideoNode;
use std::rc::Rc;
use wasm_bindgen::JsCast;
use web_sys::WebGlRenderingContext;

pub struct App {
    link: ComponentLink<Self>,
    model: Option<RenderChain>,
    node_ref: NodeRef,
    render_loop: Option<Box<dyn Task>>,
    nodes: Vec<VideoNode>,
}

pub enum Msg {
    Render(f64),
}

impl Component for App {
    type Message = Msg;
    type Properties = ();

    fn create(_: Self::Properties, link: ComponentLink<Self>) -> Self {
        info!("App started");
        let nodes = vec![
            VideoNode::new_effect(resources::effects::PURPLE),
            VideoNode::new_effect(resources::effects::TEST),
            VideoNode::new_effect(resources::effects::RESAT),
        ];
        App {
            link,
            model: None,
            node_ref: Default::default(),
            render_loop: None,
            nodes,
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

        self.schedule_next_render();

        false
    }

    fn update(&mut self, msg: Self::Message) -> ShouldRender {
        match msg {
            Msg::Render(timestamp) => self.render_gl(timestamp),
        };
        false
    }

    fn view(&self) -> Html {
        html! {
            <div class="hello">
                <h1>{"Radiance"}</h1>
                <canvas ref={self.node_ref.clone()} width=512 height=512 />
            </div>
        }
    }
}

impl App {
    fn render_gl(&mut self, time: f64) {
        let model = self.model.as_mut().unwrap();
        for node in &mut self.nodes {
            node.update_time(time / 1e3);
        }
        model.paint(&self.nodes).unwrap();

        self.schedule_next_render();
    }

    fn schedule_next_render(&mut self) {
        let render_frame = self.link.callback(Msg::Render);
        let handle = RenderService::new().request_animation_frame(render_frame);

        // A reference to the new handle must be retained for the next render to run.
        self.render_loop = Some(Box::new(handle));
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
