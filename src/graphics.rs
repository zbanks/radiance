use crate::resources;
use crate::video_node::*;
use log::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use web_sys::WebGlRenderingContext as GL;
use web_sys::{
    WebGlBuffer, WebGlFramebuffer, WebGlProgram, WebGlRenderingContext, WebGlShader, WebGlTexture,
    WebGlUniformLocation,
};

pub type ChainSize = (i32, i32);

pub struct Fbo {
    context: Rc<WebGlRenderingContext>,
    texture: WebGlTexture,
    framebuffer: WebGlFramebuffer,
}

pub struct Shader {
    context: Rc<WebGlRenderingContext>,
    program: WebGlProgram,
    fragment_shader: WebGlShader,
    vertex_shader: WebGlShader,
    pub fbo: RefCell<Fbo>,
}

pub struct ActiveShader<'a> {
    shader: &'a Shader,
}

pub struct RenderChain {
    pub context: Rc<WebGlRenderingContext>,
    size: ChainSize,
    blit_shader: Shader,
    blank_texture: WebGlTexture,
    //noise_texture: WebGlTexture,
    square_vertex_buffer: WebGlBuffer,
    pub extra_fbo: RefCell<Fbo>,

    artists: RefCell<HashMap<usize, VideoArtist>>,
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
    pub fn from_fragment_shader(context: Rc<WebGlRenderingContext>, shader_code: &str) -> Shader {
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

    pub fn begin_render<'a>(&'a self, chain: &RenderChain, fbo: Option<&Fbo>) -> ActiveShader<'a> {
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
    pub fn get_uniform_location(&self, uniform: &str) -> Option<WebGlUniformLocation> {
        return self
            .shader
            .context
            .get_uniform_location(&self.shader.program, uniform);
    }

    pub fn finish_render(self) -> () {
        self.shader.context.draw_arrays(GL::TRIANGLE_STRIP, 0, 4);
        self.shader.context.use_program(None);
        self.shader.context.bind_framebuffer(GL::FRAMEBUFFER, None);
        self.shader.context.active_texture(GL::TEXTURE0); // This resets the scene graph?
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

        let artists = Default::default();

        RenderChain {
            context,
            extra_fbo,
            blit_shader,
            blank_texture,
            square_vertex_buffer,
            size,
            artists,
        }
    }

    pub fn paint(&self, nodes: &Vec<VideoNode>) -> () {
        self.context.disable(GL::DEPTH_TEST);
        self.context.disable(GL::BLEND);
        self.context.clear_color(0.0, 0.0, 0.0, 0.0);
        self.context.clear(GL::COLOR_BUFFER_BIT);

        for node in nodes {
            self.artists
                .borrow_mut()
                .entry(node.id())
                .or_insert_with(|| node.new_artist(&self));
        }

        let artists = self.artists.borrow();
        let mut last_fbo: Option<&RefCell<Fbo>> = None;
        for node in nodes {
            let artist = artists.get(&node.id()).unwrap();
            last_fbo = artist.paint(&self, node, last_fbo);
        }

        {
            let active_shader = self.blit_shader.begin_render(self, None);

            self.bind_fbo_to_texture(GL::TEXTURE0, last_fbo);
            let loc = active_shader.get_uniform_location("iInputs");
            self.context.uniform1iv_with_i32_array(loc.as_ref(), &[0]);

            active_shader.finish_render();
        }
    }

    pub fn bind_fbo_to_texture(&self, tex: u32, fbo_ref: Option<&RefCell<Fbo>>) {
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
        /*
        for effect in &self.effect_list {
            effect.update_time(time);
        }
        */
    }
}

impl Drop for RenderChain {
    fn drop(&mut self) -> () {
        self.context.delete_buffer(Some(&self.square_vertex_buffer));
        self.context.delete_texture(Some(&self.blank_texture));
    }
}
