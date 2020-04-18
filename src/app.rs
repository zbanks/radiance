use log::*;
//use serde_derive::{Deserialize, Serialize};
//use strum::IntoEnumIterator;
//use strum_macros::{EnumIter, ToString};
//use yew::format::Json;
//use yew::services::storage::{Area, StorageService};
use wasm_bindgen::prelude::*;
use yew::prelude::*;
use yew::services::{RenderService, Task};

use wasm_bindgen::JsCast;
use web_sys::WebGlRenderingContext as GL;
use web_sys::{WebGlFramebuffer, WebGlProgram, WebGlRenderingContext, WebGlShader, WebGlTexture, WebGlBuffer};
use std::rc::Rc;

pub struct App {
    link: ComponentLink<Self>,
    model: Option<Model>,
    node_ref: NodeRef,
    render_loop: Option<Box<dyn Task>>,
}

pub struct Fbo {
    context: Rc<WebGlRenderingContext>,
    texture: WebGlTexture,
    framebuffer: WebGlFramebuffer,
}

pub struct ShaderNode {
    context: Rc<WebGlRenderingContext>,
    program: WebGlProgram,
    frag_shader: WebGlShader,
    vert_shader: WebGlShader,
    fbo: Fbo,
}

pub struct Model {
    context: Rc<WebGlRenderingContext>,
    shaders: Vec<ShaderNode>,
    blit_shader: ShaderNode,
    extra_fbo: Fbo,
    vertex_buffer: WebGlBuffer,
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
    fn render_gl(&mut self, time: f64) -> () {
        let model = self.model.as_mut().unwrap();
        let gl_context = &model.context;

        gl_context.disable(GL::DEPTH_TEST);
        gl_context.disable(GL::BLEND);
        gl_context.clear_color(0.0, 0.0, 0.0, 0.0);
        gl_context.clear(GL::COLOR_BUFFER_BIT);

        for shader in &mut model.shaders {
            //gl_context.bind_framebuffer(GL::FRAMEBUFFER, None);
            //model.extra_fbo.bind();
            gl_context.bind_framebuffer(GL::FRAMEBUFFER, Some(&model.extra_fbo.framebuffer));
            gl_context.active_texture(GL::TEXTURE0);
            gl_context.bind_texture(GL::TEXTURE_2D, None);

            gl_context.use_program(Some(&shader.program));

            let position_loc = gl_context.get_attrib_location(&shader.program, "vPosition");

            gl_context.bind_buffer(GL::ARRAY_BUFFER, Some(&model.vertex_buffer));
            gl_context.enable_vertex_attrib_array(position_loc as u32);
            gl_context.vertex_attrib_pointer_with_i32(position_loc as u32, 4, GL::FLOAT, false, 0, 0);

            gl_context.draw_arrays(
                GL::TRIANGLE_STRIP,
                0,
                4
            );
            gl_context.active_texture(GL::TEXTURE0);
            gl_context.use_program(None);
            std::mem::swap(&mut model.extra_fbo, &mut shader.fbo);
        }

        {
            gl_context.bind_framebuffer(GL::FRAMEBUFFER, None);
            gl_context.active_texture(GL::TEXTURE0);
            gl_context.bind_texture(GL::TEXTURE_2D, Some(&model.shaders.last().unwrap().fbo.texture));

            gl_context.use_program(Some(&model.blit_shader.program));

            let position_loc = gl_context.get_attrib_location(&model.blit_shader.program, "vPosition");
            gl_context.bind_buffer(GL::ARRAY_BUFFER, Some(&model.vertex_buffer));
            gl_context.enable_vertex_attrib_array(position_loc as u32);
            gl_context.vertex_attrib_pointer_with_i32(position_loc as u32, 4, GL::FLOAT, false, 0, 0);

            let texture_loc = gl_context.get_uniform_location(&model.blit_shader.program, "uSampler");
            gl_context.uniform1i(texture_loc.as_ref(), 0);

            gl_context.draw_arrays(
                GL::TRIANGLE_STRIP,
                0,
                4
            );
            gl_context.active_texture(GL::TEXTURE0);
            gl_context.use_program(None);
        }

        self.schedule_next_render();
    }

    fn schedule_next_render(&mut self) -> () {
        let render_frame = self.link.callback(|time: f64| Msg::Render(time));
        let handle = RenderService::new().request_animation_frame(render_frame);

        // A reference to the new handle must be retained for the next render to run.
        self.render_loop = Some(Box::new(handle));
    }
}

impl ShaderNode {
    fn new(context: Rc<WebGlRenderingContext>) -> ShaderNode {
        Self::from_fragment_shader(context, 
            r#"

            precision mediump float;
            varying highp vec2 uv;

            void main() {
                gl_FragColor = vec4(0.5, uv.y, 1.0, 1.0);
            }
        "#)
    }

    fn from_fragment_shader(context: Rc<WebGlRenderingContext>, shader_code: &str) -> ShaderNode {
        info!("Compiling shaders");
        let vert_shader = Self::compile_shader(
            &context,
            WebGlRenderingContext::VERTEX_SHADER,
            r#"
            attribute vec4 vPosition;
            varying highp vec2 uv;

            void main() {
                gl_Position = vec4(vPosition.xy, 0., 1.);
                uv = vPosition.zw;
            }
        "#,
        )
        .unwrap();
        let frag_shader = Self::compile_shader(
            &context,
            WebGlRenderingContext::FRAGMENT_SHADER,
            shader_code,
        )
        .unwrap();
        let program = Self::link_program(&context, &vert_shader, &frag_shader).unwrap();
        let fbo = Fbo::new(Rc::clone(&context));

        ShaderNode { context, program, frag_shader, vert_shader, fbo }
    }

    fn compile_shader(
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

    fn link_program(
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
}

impl Drop for ShaderNode {
    fn drop(&mut self) -> () {
        self.context.delete_shader(Some(&self.frag_shader));
        self.context.delete_shader(Some(&self.vert_shader));
        self.context.delete_program(Some(&self.program));
    }
}

impl Fbo {
    fn new(context: Rc<WebGlRenderingContext>) -> Fbo {
        let texture = context.create_texture().unwrap();
        context.bind_texture(GL::TEXTURE_2D, Some(&texture));
        context.tex_image_2d_with_i32_and_i32_and_i32_and_format_and_type_and_opt_u8_array(
            GL::TEXTURE_2D,                  // target
            0,                               // level
            GL::RGBA as i32,                 // internal format
            context.drawing_buffer_width(),  // width
            context.drawing_buffer_height(), // height
            0,                               // border
            GL::RGBA,                        // format
            GL::UNSIGNED_BYTE,               // _type
            None,                            // pixels
        );
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE as i32);

        let framebuffer = context.create_framebuffer().unwrap();
        context.bind_framebuffer(GL::FRAMEBUFFER, Some(&framebuffer));
        context.framebuffer_texture_2d(
            GL::FRAMEBUFFER,
            GL::COLOR_ATTACHMENT0,
            GL::TEXTURE_2D,
            Some(&texture),
            0, /* level */
        );

        Fbo {
            context,
            texture,
            framebuffer,
        }
    }

    fn bind(&self) -> () {
        self.context.bind_framebuffer(GL::FRAMEBUFFER, Some(&self.framebuffer));
    }
}

impl Drop for Fbo {
    fn drop(&mut self) -> () {
        self.context.delete_texture(Some(&self.texture));
        self.context.delete_framebuffer(Some(&self.framebuffer));
    }
}

impl Model {
    fn new(context: WebGlRenderingContext) -> Model {
        info!("New model & context!");
        let context = Rc::new(context);
        let shaders = vec![ShaderNode::new(Rc::clone(&context))];
        let extra_fbo = Fbo::new(Rc::clone(&context));

        let blit_shader = ShaderNode::from_fragment_shader(Rc::clone(&context), r#"
            varying highp vec2 uv;
            uniform sampler2D uSampler;
            void main() {
                gl_FragColor = texture2D(uSampler, uv);
            }
        "#);

        let vertices: [f32; 16] = [
             1.0,  1.0, 1.0, 1.0,
            -1.0,  1.0, 0.0, 1.0,
             1.0, -1.0, 1.0, 0.0,
            -1.0, -1.0, 0.0, 0.0,
        ];

        let vertex_buffer = context .create_buffer() .ok_or("failed to create buffer") .unwrap();
        context.bind_buffer(GL::ARRAY_BUFFER, Some(&vertex_buffer));

        // Note: there is an alternate way to do this with Float32Array::view
        // that is "faster" and avoids copying this buffer; but this buffer is so
        // small I don't think it's worth the use of `unsafe`
        let vert_array = js_sys::Float32Array::from(&vertices as &[f32]);
        context.buffer_data_with_opt_array_buffer(
            GL::ARRAY_BUFFER,
            Some(&vert_array.buffer()),
            GL::STATIC_DRAW,
        );

        Model {
            context,
            shaders,
            extra_fbo,
            vertex_buffer,
            blit_shader,
        }
    }
}

impl Drop for Model {
    fn drop(&mut self) -> () {
        self.context.delete_buffer(Some(&self.vertex_buffer));
    }
}
