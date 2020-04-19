use crate::resources;
use log::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use web_sys::WebGlRenderingContext as GL;
use web_sys::{
    WebGlBuffer, WebGlFramebuffer, WebGlProgram, WebGlRenderingContext, WebGlShader, WebGlTexture,
    WebGlUniformLocation,
};

struct Fbo {
    context: Rc<WebGlRenderingContext>,
    texture: WebGlTexture,
    framebuffer: WebGlFramebuffer,
}

struct Shader {
    context: Rc<WebGlRenderingContext>,
    program: WebGlProgram,
    fragment_shader: WebGlShader,
    vertex_shader: WebGlShader,
    fbo: RefCell<Fbo>,
}

struct ActiveShader<'a> {
    shader: &'a Shader,
}

struct EffectNode {
    context: Rc<WebGlRenderingContext>,
    shader_passes: Vec<Shader>,
    properties: HashMap<String, String>,
    intensity: RefCell<f64>,
    time: RefCell<f64>,
    intensity_integral: RefCell<f64>,
}

pub struct RenderChain {
    context: Rc<WebGlRenderingContext>,
    size: (i32, i32),
    blit_shader: Shader,
    blank_texture: WebGlTexture,
    //noise_texture: WebGlTexture,
    square_vertex_buffer: WebGlBuffer,
    extra_fbo: RefCell<Fbo>,

    // XXX This goes elsewhere
    effect_list: Vec<EffectNode>,
}

trait VideoNode {
    fn paint<'a>(
        &'a self,
        chain: &'a RenderChain,
        on_fbo: Option<&'a RefCell<Fbo>>,
    ) -> Option<&'a RefCell<Fbo>>;
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
            0, // level
        );

        Fbo {
            context,
            texture,
            framebuffer,
        }
    }
}

impl Drop for Fbo {
    fn drop(&mut self) -> () {
        self.context.delete_texture(Some(&self.texture));
        self.context.delete_framebuffer(Some(&self.framebuffer));
    }
}

impl Shader {
    fn from_fragment_shader(context: Rc<WebGlRenderingContext>, shader_code: &str) -> Shader {
        info!("Compiling shaders");
        let vertex_shader = Self::compile_shader(
            &context,
            WebGlRenderingContext::VERTEX_SHADER,
            resources::glsl::PLAIN_VERTEX,
        )
        .unwrap();
        let fragment_shader = Self::compile_shader(
            &context,
            WebGlRenderingContext::FRAGMENT_SHADER,
            shader_code,
        )
        .unwrap();
        let program = Self::link_program(&context, &vertex_shader, &fragment_shader).unwrap();
        let fbo = RefCell::new(Fbo::new(Rc::clone(&context)));

        Shader {
            context,
            program,
            fragment_shader,
            vertex_shader,
            fbo,
        }
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

    fn render(&self, chain: &RenderChain, fbo: Option<&Fbo>) -> () {
        let loc = self.context.get_uniform_location(&self.program, "iInputs");
        self.context.uniform1iv_with_i32_array(loc.as_ref(), &[0]);
    }

    fn begin_render<'a>(&'a self, chain: &RenderChain, fbo: Option<&Fbo>) -> ActiveShader<'a> {
        self.context.use_program(Some(&self.program));

        let loc = self.context.get_attrib_location(&self.program, "vPosition");
        self.context
            .bind_buffer(GL::ARRAY_BUFFER, Some(&chain.square_vertex_buffer));
        self.context.enable_vertex_attrib_array(loc as u32);
        self.context
            .vertex_attrib_pointer_with_i32(loc as u32, 4, GL::FLOAT, false, 0, 0);

        self.context
            .bind_framebuffer(GL::FRAMEBUFFER, fbo.map(|f| &f.framebuffer));

        ActiveShader { shader: &self }
    }
}

impl<'a> ActiveShader<'a> {
    fn get_uniform_location(&self, uniform: &str) -> Option<WebGlUniformLocation> {
        return self
            .shader
            .context
            .get_uniform_location(&self.shader.program, uniform);
    }

    fn finish_render(self) -> () {
        self.shader.context.draw_arrays(GL::TRIANGLE_STRIP, 0, 4);
        self.shader.context.use_program(None);
        self.shader.context.bind_framebuffer(GL::FRAMEBUFFER, None);
        self.shader.context.active_texture(GL::TEXTURE0); // This resets the scene graph?
    }
}

impl EffectNode {
    fn new(context: Rc<WebGlRenderingContext>, program: &str) -> EffectNode {
        let mut shader_passes = Vec::new();
        let mut header_source = String::from(resources::glsl::EFFECT_HEADER);
        let mut source = String::new();
        let mut properties = HashMap::new();

        source.push_str(&header_source);
        source.push_str("\n#line 1\n");

        for line in program.split('\n') {
            let mut terms = line.trim().splitn(3, ' ');
            let head = terms.next();
            match head {
                Some("#property") => {
                    let key = terms.next().unwrap().to_string();
                    let value = terms.next().unwrap().to_string();
                    properties.insert(key, value);
                }
                Some("#buffershader") => {
                    shader_passes.push(Shader::from_fragment_shader(Rc::clone(&context), &source));
                    source = String::new();
                    source.push_str(&header_source);
                    source.push_str("\n#line 1\n"); // XXX
                }
                _ => {
                    source.push_str(&line);
                }
            }
            source.push_str("\n");
        }
        shader_passes.push(Shader::from_fragment_shader(Rc::clone(&context), &source));
        info!("Loaded effect: {:?}", properties);

        EffectNode {
            context,
            shader_passes,
            properties,
            time: RefCell::new(0.0),
            intensity: RefCell::new(0.8),
            intensity_integral: RefCell::new(0.0),
        }
    }

    fn update_time(&self, time: f64) -> () {
        let mut t = self.time.borrow_mut();
        *t = time;
    }
}

impl VideoNode for EffectNode {
    fn paint<'a>(
        &'a self,
        chain: &'a RenderChain,
        on_fbo: Option<&'a RefCell<Fbo>>,
    ) -> Option<&'a RefCell<Fbo>> {
        let mut last_fbo = on_fbo;

        for shader in self.shader_passes.iter().rev() {
            {
                let active_shader = shader.begin_render(chain, Some(&chain.extra_fbo.borrow()));

                chain.bind_fbo_to_texture(GL::TEXTURE0, on_fbo);
                let loc = active_shader.get_uniform_location("iInputs");
                self.context.uniform1iv_with_i32_array(loc.as_ref(), &[0]);

                let mut channels: Vec<i32> = vec![];
                for (i, shader) in self.shader_passes.iter().enumerate() {
                    chain.bind_fbo_to_texture(GL::TEXTURE0 + 1 + i as u32, Some(&shader.fbo));
                    channels.push(1 + i as i32);
                }
                let loc = active_shader.get_uniform_location("iChannel");
                self.context
                    .uniform1iv_with_i32_array(loc.as_ref(), &channels);

                let loc = active_shader.get_uniform_location("iIntensity");
                self.context
                    .uniform1f(loc.as_ref(), *self.intensity.borrow() as f32);

                let loc = self.context.get_uniform_location(&shader.program, "iTime");
                self.context
                    .uniform1f(loc.as_ref(), *self.time.borrow() as f32);

                let loc = self
                    .context
                    .get_uniform_location(&shader.program, "iResolution");
                self.context.uniform2f(
                    loc.as_ref(),
                    self.context.drawing_buffer_width() as f32,
                    self.context.drawing_buffer_height() as f32,
                );

                active_shader.finish_render();
            }

            chain.extra_fbo.swap(&shader.fbo);
            last_fbo = Some(&shader.fbo);
        }

        last_fbo
    }
}

impl Drop for Shader {
    fn drop(&mut self) -> () {
        self.context.delete_shader(Some(&self.fragment_shader));
        self.context.delete_shader(Some(&self.vertex_shader));
        self.context.delete_program(Some(&self.program));
    }
}

impl RenderChain {
    pub fn new(context: Rc<WebGlRenderingContext>) -> RenderChain {
        let size = (
            context.drawing_buffer_width(),
            context.drawing_buffer_height(),
        );
        let extra_fbo = RefCell::new(Fbo::new(Rc::clone(&context)));
        let effect_list = vec![
            EffectNode::new(Rc::clone(&context), resources::effects::PURPLE),
            EffectNode::new(Rc::clone(&context), resources::effects::TEST),
            EffectNode::new(Rc::clone(&context), resources::effects::RESAT),
        ];

        let blit_shader =
            Shader::from_fragment_shader(Rc::clone(&context), resources::glsl::PLAIN_FRAGMENT);

        let blank_texture = context.create_texture().unwrap();
        context.bind_texture(GL::TEXTURE_2D, Some(&blank_texture));
        context.tex_image_2d_with_i32_and_i32_and_i32_and_format_and_type_and_opt_u8_array(
            GL::TEXTURE_2D,        // target
            0,                     // level
            GL::RGBA as i32,       // internal format
            1,                     // width
            1,                     // height
            0,                     // border
            GL::RGBA,              // format
            GL::UNSIGNED_BYTE,     // _type
            Some(&[0, 0, 0, 255]), // pixels
        );
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE as i32);

        let vertices: [f32; 16] = [
            1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 0.0, 1.0, 1.0, -1.0, 1.0, 0.0, -1.0, -1.0, 0.0, 0.0,
        ];

        let square_vertex_buffer = context
            .create_buffer()
            .ok_or("failed to create buffer")
            .unwrap();
        context.bind_buffer(GL::ARRAY_BUFFER, Some(&square_vertex_buffer));

        // Note: there is an alternate way to do this with Float32Array::view
        // that is "faster" and avoids copying this buffer; but this buffer is so
        // small I don't think it's worth the use of `unsafe`
        let vert_array = js_sys::Float32Array::from(&vertices as &[f32]);
        context.buffer_data_with_opt_array_buffer(
            GL::ARRAY_BUFFER,
            Some(&vert_array.buffer()),
            GL::STATIC_DRAW,
        );

        info!("New RenderChain: ({}, {})", size.0, size.1);

        RenderChain {
            context,
            extra_fbo,
            blit_shader,
            blank_texture,
            square_vertex_buffer,
            size,
            effect_list,
        }
    }

    pub fn paint(&self) -> () {
        self.context.disable(GL::DEPTH_TEST);
        self.context.disable(GL::BLEND);
        self.context.clear_color(0.0, 0.0, 0.0, 0.0);
        self.context.clear(GL::COLOR_BUFFER_BIT);

        let mut last_fbo = None;
        for effect in &self.effect_list {
            last_fbo = effect.paint(&self, last_fbo);
        }

        {
            let active_shader = self.blit_shader.begin_render(self, None);

            self.bind_fbo_to_texture(GL::TEXTURE0, last_fbo);
            let loc = active_shader.get_uniform_location("iInputs");
            self.context.uniform1iv_with_i32_array(loc.as_ref(), &[0]);

            active_shader.finish_render();
        }
    }

    fn bind_fbo_to_texture(&self, tex: u32, fbo_ref: Option<&RefCell<Fbo>>) {
        self.context.active_texture(tex);
        if let Some(fbo) = fbo_ref {
            self.context
                .bind_texture(GL::TEXTURE_2D, Some(&fbo.borrow().texture));
        } else {
            self.context
                .bind_texture(GL::TEXTURE_2D, Some(&self.blank_texture));
        }
    }

    pub fn update_time(&self, time: f64) {
        for effect in &self.effect_list {
            effect.update_time(time);
        }
    }
}

impl Drop for RenderChain {
    fn drop(&mut self) -> () {
        self.context.delete_buffer(Some(&self.square_vertex_buffer));
        self.context.delete_texture(Some(&self.blank_texture));
    }
}
