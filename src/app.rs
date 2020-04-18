use log::*;
//use serde_derive::{Deserialize, Serialize};
//use strum::IntoEnumIterator;
//use strum_macros::{EnumIter, ToString};
//use yew::format::Json;
//use yew::services::storage::{Area, StorageService};
use yew::services::{RenderService,Task};
use yew::prelude::*;
use wasm_bindgen::prelude::*;

use wasm_bindgen::JsCast;
use web_sys::{WebGlProgram, WebGlRenderingContext, WebGlShader};

pub struct App {
    link: ComponentLink<Self>,
    model: Model,
    node_ref: NodeRef,
    gl_context: Option<WebGlRenderingContext>,
    render_loop: Option<Box<dyn Task>>,
}

pub struct Model {

}

pub enum Msg {
    Render(f64),
}

impl Component for App {
    type Message = Msg;
    type Properties = ();

    fn create(_: Self::Properties, link: ComponentLink<Self>) -> Self {
        let model = Model {};
        App { link, model, node_ref: Default::default(), gl_context: None, render_loop: None }
    }

    fn mounted(&mut self) -> ShouldRender {
        let canvas: web_sys::HtmlCanvasElement = self.node_ref.cast().unwrap();
        let gl_context = canvas.get_context("webgl").expect("WebGL not supported").unwrap().dyn_into::<WebGlRenderingContext>().unwrap();
        self.gl_context = Some(gl_context);

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
    fn render_gl(&mut self, time: f64) -> () {
        let gl_context = self.gl_context.as_ref().unwrap();
        let vert_shader = compile_shader(
            gl_context,
            WebGlRenderingContext::VERTEX_SHADER,
            r#"
            attribute vec4 position;
            void main() {
                gl_Position = position;
            }
        "#,
        ).unwrap();
        let frag_shader = compile_shader(
            gl_context,
            WebGlRenderingContext::FRAGMENT_SHADER,
            r#"
            void main() {
                gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0);
            }
        "#,
        ).unwrap();
        let program = link_program(&gl_context, &vert_shader, &frag_shader).unwrap();
        gl_context.use_program(Some(&program));

        let vertices: [f32; 9] = [-0.7, f32::sin(time as f32 / 1000.), 0.0, 0.7, -0.7, 0.0, 0.0, 0.7, 0.0];

        let buffer = gl_context.create_buffer().ok_or("failed to create buffer").unwrap();
        gl_context.bind_buffer(WebGlRenderingContext::ARRAY_BUFFER, Some(&buffer));

        // Note that `Float32Array::view` is somewhat dangerous (hence the
        // `unsafe`!). This is creating a raw view into our module's
        // `WebAssembly.Memory` buffer, but if we allocate more pages for ourself
        // (aka do a memory allocation in Rust) it'll cause the buffer to change,
        // causing the `Float32Array` to be invalid.
        //
        // As a result, after `Float32Array::view` we have to be very careful not to
        // do any memory allocations before it's dropped.
        unsafe {
            let vert_array = js_sys::Float32Array::view(&vertices);

            gl_context.buffer_data_with_array_buffer_view(
                WebGlRenderingContext::ARRAY_BUFFER,
                &vert_array,
                WebGlRenderingContext::STATIC_DRAW,
            );
        }

        gl_context.vertex_attrib_pointer_with_i32(0, 3, WebGlRenderingContext::FLOAT, false, 0, 0);
        gl_context.enable_vertex_attrib_array(0);

        gl_context.clear_color(0.0, 0.0, 0.0, 1.0);
        gl_context.clear(WebGlRenderingContext::COLOR_BUFFER_BIT);

        gl_context.draw_arrays(
            WebGlRenderingContext::TRIANGLES,
            0,
            (vertices.len() / 3) as i32,
        );

        self.schedule_next_render();
    }

    fn schedule_next_render(&mut self) -> () {
        let render_frame = self.link.callback(|time: f64| Msg::Render(time));
        let handle = RenderService::new().request_animation_frame(render_frame);

        // A reference to the new handle must be retained for the next render to run.
        self.render_loop = Some(Box::new(handle));
    }
}

pub fn compile_shader(
    context: &WebGlRenderingContext,
    shader_type: u32,
    source: &str,
) -> Result<WebGlShader, String> {
    let shader = context
        .create_shader(shader_type)
        .ok_or_else(|| String::from("Unable to create shader object"))?;
    context.shader_source(&shader, source);
    context.compile_shader(&shader);

    if context
        .get_shader_parameter(&shader, WebGlRenderingContext::COMPILE_STATUS)
        .as_bool()
        .unwrap_or(false)
    {
        Ok(shader)
    } else {
        Err(context
            .get_shader_info_log(&shader)
            .unwrap_or_else(|| String::from("Unknown error creating shader")))
    }
}

pub fn link_program(
    context: &WebGlRenderingContext,
    vert_shader: &WebGlShader,
    frag_shader: &WebGlShader,
) -> Result<WebGlProgram, String> {
    let program = context
        .create_program()
        .ok_or_else(|| String::from("Unable to create shader object"))?;

    context.attach_shader(&program, vert_shader);
    context.attach_shader(&program, frag_shader);
    context.link_program(&program);

    if context
        .get_program_parameter(&program, WebGlRenderingContext::LINK_STATUS)
        .as_bool()
        .unwrap_or(false)
    {
        Ok(program)
    } else {
        Err(context
            .get_program_info_log(&program)
            .unwrap_or_else(|| String::from("Unknown error creating program object")))
    }
}

impl Model {

}
