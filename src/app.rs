use log::*;
//use serde_derive::{Deserialize, Serialize};
//use strum::IntoEnumIterator;
//use strum_macros::{EnumIter, ToString};
//use yew::format::Json;
//use yew::services::storage::{Area, StorageService};
use yew::prelude::*;
use yew::services::{RenderService, Task};

use crate::graphics::Model;
use wasm_bindgen::JsCast;
use web_sys::WebGlRenderingContext;

pub struct App {
    link: ComponentLink<Self>,
    model: Option<Model>,
    node_ref: NodeRef,
    render_loop: Option<Box<dyn Task>>,
}

pub enum Msg {
    Render(f64),
}

impl Component for App {
    type Message = Msg;
    type Properties = ();

    fn create(_: Self::Properties, link: ComponentLink<Self>) -> Self {
        App {
            link,
            model: None,
            node_ref: Default::default(),
            render_loop: None,
        }
    }

    fn mounted(&mut self) -> ShouldRender {
        let canvas: web_sys::HtmlCanvasElement = self.node_ref.cast().unwrap();
        let gl_context = canvas
            .get_context("webgl")
            .expect("WebGL not supported")
            .unwrap()
            .dyn_into::<WebGlRenderingContext>()
            .unwrap();
        self.model = Some(Model::new(gl_context));

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
        info!("rendered!");
        html! {
            <div class="hello">
                <h1>{"Radiance"}</h1>
                <canvas ref={self.node_ref.clone()} />
            </div>
        }
    }
}

impl App {
    fn render_gl(&mut self, _time: f64) -> () {
        let model = self.model.as_mut().unwrap();
        model.paint();

        self.schedule_next_render();
    }

    fn schedule_next_render(&mut self) -> () {
        let render_frame = self.link.callback(|time: f64| Msg::Render(time));
        let handle = RenderService::new().request_animation_frame(render_frame);

        // A reference to the new handle must be retained for the next render to run.
        self.render_loop = Some(Box::new(handle));
    }
}
