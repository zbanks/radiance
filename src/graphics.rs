use crate::audio::AudioAnalysis;
use crate::err::{Error, Result};
use crate::resources;
use crate::video_node::{IVideoNode, VideoNode, VideoNodeId};

use log::*;
use std::borrow::Borrow;
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
    pub texture: WebGlTexture,
    framebuffer: WebGlFramebuffer,
}

pub struct Shader {
    context: Rc<WebGlRenderingContext>,
    program: WebGlProgram,
    fragment_shader: WebGlShader,
    vertex_shader: WebGlShader,
    vposition_location: u32,
    uniform_locations: RefCell<HashMap<String, Option<WebGlUniformLocation>>>,
}

pub struct ActiveShader<'a> {
    shader: &'a Shader,
}

pub struct RenderChain {
    pub context: Rc<WebGlRenderingContext>,
    pub size: ChainSize,
    pub extra_fbo: RefCell<Rc<Fbo>>,

    pub blit_shader: Shader,
    blank_texture: WebGlTexture,
    pub noise_texture: WebGlTexture,
    square_vertex_buffer: WebGlBuffer,

    audio: AudioAnalysis,

    buffer_fbos: RefCell<HashMap<VideoNodeId, Vec<Rc<Fbo>>>>,
    output_fbos: RefCell<HashMap<VideoNodeId, Rc<Fbo>>>,
}

/// This represents a WebGL framebuffer + texture pair, where the
/// texture is the render target of the framebuffer
impl Fbo {
    fn new(context: Rc<WebGlRenderingContext>, size: ChainSize) -> Result<Fbo> {
        let texture = context.create_texture().ok_or("Unable to create texture")?;
        context.bind_texture(GL::TEXTURE_2D, Some(&texture));
        context.tex_image_2d_with_i32_and_i32_and_i32_and_format_and_type_and_opt_u8_array(
            GL::TEXTURE_2D,    // target
            0,                 // level
            GL::RGBA as i32,   // internal format
            size.0,            // width
            size.1,            // height
            0,                 // border
            GL::RGBA,          // format
            GL::UNSIGNED_BYTE, // _type
            None,              // pixels
        )?;
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE as i32);

        let framebuffer = context
            .create_framebuffer()
            .ok_or("Unable to create framebuffer")?;
        context.bind_framebuffer(GL::FRAMEBUFFER, Some(&framebuffer));
        context.framebuffer_texture_2d(
            GL::FRAMEBUFFER,
            GL::COLOR_ATTACHMENT0,
            GL::TEXTURE_2D,
            Some(&texture),
            0, // level
        );

        Ok(Fbo {
            context,
            texture,
            framebuffer,
        })
    }

    pub fn image_video_element(&self, video: &web_sys::HtmlVideoElement) -> Result<()> {
        self.context.bind_framebuffer(GL::FRAMEBUFFER, None);
        self.context
            .bind_texture(GL::TEXTURE_2D, Some(&self.texture));
        self.context.pixel_storei(GL::UNPACK_FLIP_Y_WEBGL, 1);
        self.context.tex_image_2d_with_u32_and_u32_and_video(
            GL::TEXTURE_2D,    // target
            0,                 // level
            GL::RGBA as i32,   // internal format
            GL::RGBA,          // format
            GL::UNSIGNED_BYTE, // type
            video,             // source
        )?;
        self.context.bind_texture(GL::TEXTURE_2D, None);
        Ok(())
    }
}

impl Drop for Fbo {
    fn drop(&mut self) {
        self.context.delete_texture(Some(&self.texture));
        self.context.delete_framebuffer(Some(&self.framebuffer));
    }
}

impl Shader {
    fn from_fragment_shader(
        context: Rc<WebGlRenderingContext>,
        shader_code: &str,
    ) -> Result<Shader> {
        info!("Compiling shaders");
        let vertex_shader = Self::compile_shader(
            &context,
            WebGlRenderingContext::VERTEX_SHADER,
            resources::glsl::PLAIN_VERTEX,
        )?;
        let fragment_shader = Self::compile_shader(
            &context,
            WebGlRenderingContext::FRAGMENT_SHADER,
            shader_code,
        )?;
        let program = Self::link_program(&context, &vertex_shader, &fragment_shader)?;
        let vposition_location = context.get_attrib_location(&program, "vPosition") as u32;

        Ok(Shader {
            context,
            program,
            fragment_shader,
            vertex_shader,
            vposition_location,
            uniform_locations: Default::default(),
        })
    }

    fn compile_shader(
        context: &WebGlRenderingContext,
        shader_type: u32,
        source: &str,
    ) -> Result<WebGlShader> {
        let shader = context
            .create_shader(shader_type)
            .ok_or("Unable to create shader object")?;
        context.shader_source(&shader, source);
        context.compile_shader(&shader);

        if context
            .get_shader_parameter(&shader, WebGlRenderingContext::COMPILE_STATUS)
            .as_bool()
            .unwrap_or(false)
        {
            Ok(shader)
        } else {
            Err(Error::glsl(
                context
                    .get_shader_info_log(&shader)
                    .unwrap_or_else(|| String::from("Unknown error creating shader")),
            ))
        }
    }

    fn link_program(
        context: &WebGlRenderingContext,
        vert_shader: &WebGlShader,
        frag_shader: &WebGlShader,
    ) -> Result<WebGlProgram> {
        let program = context
            .create_program()
            .ok_or("Unable to create shader object")?;

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
            Err(Error::glsl(
                context
                    .get_program_info_log(&program)
                    .unwrap_or_else(|| String::from("Unknown error creating program object")),
            ))
        }
    }

    pub fn begin_render<'a>(&'a self, chain: &RenderChain, fbo: Option<&Fbo>) -> ActiveShader<'a> {
        self.context.use_program(Some(&self.program));

        self.context
            .bind_buffer(GL::ARRAY_BUFFER, Some(&chain.square_vertex_buffer));
        self.context
            .enable_vertex_attrib_array(self.vposition_location);
        self.context.vertex_attrib_pointer_with_i32(
            self.vposition_location,
            4,
            GL::FLOAT,
            false,
            0,
            0,
        );

        self.context
            .bind_framebuffer(GL::FRAMEBUFFER, fbo.map(|f| &f.framebuffer));

        ActiveShader { shader: &self }
    }

    pub fn get_uniform_location(&self, uniform: &str) -> Option<WebGlUniformLocation> {
        self.uniform_locations
            .borrow_mut()
            .entry(uniform.to_string())
            .or_insert_with(|| self.context.get_uniform_location(&self.program, uniform))
            .clone()
    }
}

impl<'a> ActiveShader<'a> {
    pub fn set_uniform1f(&self, uniform: &str, value: f32) {
        let loc = self.shader.get_uniform_location(uniform);
        self.shader.context.uniform1f(loc.as_ref(), value);
    }

    pub fn set_uniform2f(&self, uniform: &str, value: (f32, f32)) {
        let loc = self.shader.get_uniform_location(uniform);
        self.shader
            .context
            .uniform2f(loc.as_ref(), value.0, value.1);
    }

    pub fn set_uniform4fv(&self, uniform: &str, value: &[f32]) {
        let loc = self.shader.get_uniform_location(uniform);
        self.shader
            .context
            .uniform4fv_with_f32_array(loc.as_ref(), value);
    }

    pub fn set_uniform1i(&self, uniform: &str, value: i32) {
        let loc = self.shader.get_uniform_location(uniform);
        self.shader.context.uniform1i(loc.as_ref(), value);
    }

    pub fn set_uniform1iv(&self, uniform: &str, value: &[i32]) {
        let loc = self.shader.get_uniform_location(uniform);
        self.shader
            .context
            .uniform1iv_with_i32_array(loc.as_ref(), value);
    }

    pub fn finish_render(self) {
        // Perform the render
        self.shader.context.draw_arrays(GL::TRIANGLE_STRIP, 0, 4);

        // Clean up
        self.shader.context.use_program(None);
        self.shader.context.bind_framebuffer(GL::FRAMEBUFFER, None);
        self.shader.context.active_texture(GL::TEXTURE0); // This resets the scene graph?
    }
}

impl Drop for Shader {
    fn drop(&mut self) {
        self.context.delete_shader(Some(&self.fragment_shader));
        self.context.delete_shader(Some(&self.vertex_shader));
        self.context.delete_program(Some(&self.program));
    }
}

impl RenderChain {
    pub fn new(context: Rc<WebGlRenderingContext>, size: ChainSize) -> Result<RenderChain> {
        let extra_fbo = RefCell::new(Rc::new(Fbo::new(Rc::clone(&context), size)?));

        let blit_shader =
            Shader::from_fragment_shader(Rc::clone(&context), resources::glsl::PLAIN_FRAGMENT)?;

        context.disable(GL::DEPTH_TEST);
        context.disable(GL::BLEND);
        context.enable(GL::SCISSOR_TEST);
        context.clear_color(0.0, 0.0, 0.0, 0.0);

        let blank_texture = context.create_texture().ok_or("Unable to create texture")?;
        context.bind_texture(GL::TEXTURE_2D, Some(&blank_texture));
        context.tex_image_2d_with_i32_and_i32_and_i32_and_format_and_type_and_opt_u8_array(
            GL::TEXTURE_2D,      // target
            0,                   // level
            GL::RGBA as i32,     // internal format
            1,                   // width
            1,                   // height
            0,                   // border
            GL::RGBA,            // format
            GL::UNSIGNED_BYTE,   // _type
            Some(&[0, 0, 0, 0]), // pixels
        )?;
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::CLAMP_TO_EDGE as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::CLAMP_TO_EDGE as i32);

        let noise_pixels: Vec<u8> = (0..(size.0 * size.1 * 4))
            .map(|_| (js_sys::Math::random() * 256.) as u8)
            .collect();
        let noise_texture = context.create_texture().ok_or("Unable to create texture")?;
        context.bind_texture(GL::TEXTURE_2D, Some(&noise_texture));
        context.tex_image_2d_with_i32_and_i32_and_i32_and_format_and_type_and_opt_u8_array(
            GL::TEXTURE_2D,      // target
            0,                   // level
            GL::RGBA as i32,     // internal format
            size.0,              // width
            size.1,              // height
            0,                   // border
            GL::RGBA,            // format
            GL::UNSIGNED_BYTE,   // _type
            Some(&noise_pixels), // pixels
        )?;
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MAG_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_MIN_FILTER, GL::LINEAR as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_S, GL::REPEAT as i32);
        context.tex_parameteri(GL::TEXTURE_2D, GL::TEXTURE_WRAP_T, GL::REPEAT as i32);

        let vertices: [f32; 16] = [
            1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 0.0, 1.0, 1.0, -1.0, 1.0, 0.0, -1.0, -1.0, 0.0, 0.0,
        ];

        let square_vertex_buffer = context.create_buffer().ok_or("failed to create buffer")?;
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

        Ok(RenderChain {
            context,
            extra_fbo,
            blit_shader,
            blank_texture,
            noise_texture,
            square_vertex_buffer,
            audio: Default::default(),
            size,
            buffer_fbos: Default::default(),
            output_fbos: Default::default(),
        })
    }

    pub fn compile_fragment_shader(&self, source: &str) -> Result<Shader> {
        Shader::from_fragment_shader(Rc::clone(&self.context), source)
    }

    pub fn node_fbo(&self, node: &VideoNode) -> Option<Rc<Fbo>> {
        self.output_fbos
            .borrow()
            .get(&node.id())
            .map(|n| Rc::clone(n))
    }

    pub fn audio(&self) -> &AudioAnalysis {
        &self.audio
    }

    pub fn set_audio(&mut self, audio: AudioAnalysis) {
        self.audio = audio
    }

    #[allow(dead_code)]
    pub fn resize(&mut self, size: ChainSize) {
        // TODO: This doesn't seem to quite work? It's more of an archetectural proof-of-concept
        // TODO: Better error handling (unwrap)
        let mut new_chain = RenderChain::new(Rc::clone(&self.context), size).unwrap();
        for (id, old_fbos) in self.buffer_fbos.borrow().iter() {
            let new_fbos = old_fbos
                .iter()
                .map(|old_fbo| {
                    let new_fbo = Fbo::new(Rc::clone(&new_chain.context), size).unwrap();
                    let active_shader = new_chain
                        .blit_shader
                        .begin_render(&new_chain, Some(&new_fbo));
                    self.bind_fbo_to_texture(GL::TEXTURE0, Some(old_fbo));
                    active_shader.set_uniform1iv("iInputs", &[0]);
                    active_shader.finish_render();

                    Rc::new(new_fbo)
                })
                .collect();
            new_chain.buffer_fbos.borrow_mut().insert(*id, new_fbos);
        }
        std::mem::swap(self, &mut new_chain);
    }

    #[allow(clippy::needless_lifetimes)]
    pub fn pre_render<'a, I>(&'a self, nodes: I, time: f64)
    where
        I: Iterator<Item = &'a mut VideoNode>,
    {
        self.output_fbos.borrow_mut().clear();
        for node in nodes {
            self.buffer_fbos
                .borrow_mut()
                .entry(node.id())
                .or_default()
                .resize_with(node.n_buffers(), || {
                    Rc::new(Fbo::new(Rc::clone(&self.context), self.size).unwrap())
                });
            node.pre_render(self, time / 1e3);
        }
    }

    pub fn render_node(&self, node: &VideoNode, inputs: &[Option<Rc<Fbo>>]) {
        assert!(inputs.len() == node.n_inputs());
        let output_fbo = node.render(
            self,
            inputs,
            self.buffer_fbos
                .borrow_mut()
                .get_mut(&node.id())
                .unwrap()
                .as_mut_slice(),
        );
        if let Some(output) = output_fbo {
            self.output_fbos.borrow_mut().insert(node.id(), output);
        }
    }

    pub fn set_drawing_rect(&self, x: i32, y: i32, w: i32, h: i32) {
        self.context.viewport(x, y, w, h);
        self.context.scissor(x, y, w, h);
    }

    pub fn clear(&self) {
        self.context.clear(GL::COLOR_BUFFER_BIT);
    }

    pub fn paint(&self, node: &VideoNode) -> Result<()> {
        let active_shader = self.blit_shader.begin_render(self, None);
        self.bind_fbo_to_texture(
            GL::TEXTURE0,
            self.output_fbos
                .borrow()
                .get(&node.id())
                .map(|x| x.borrow()),
        );
        active_shader.set_uniform1iv("iInputs", &[0]);
        active_shader.finish_render();
        Ok(())
    }

    pub fn bind_fbo_to_texture(&self, tex: u32, fbo_ref: Option<&Fbo>) {
        self.context.active_texture(tex);
        if let Some(fbo) = fbo_ref {
            self.context
                .bind_texture(GL::TEXTURE_2D, Some(&fbo.texture));
        } else {
            self.context
                .bind_texture(GL::TEXTURE_2D, Some(&self.blank_texture));
        }
    }
}

impl Drop for RenderChain {
    fn drop(&mut self) {
        info!("Destroyed render chain {:?}", self.size);
        self.context.delete_buffer(Some(&self.square_vertex_buffer));
        self.context.delete_texture(Some(&self.blank_texture));
    }
}
