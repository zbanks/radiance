use log::*;
use std::cell::RefCell;
use std::rc::Rc;
use web_sys::WebGlRenderingContext as GL;
use web_sys::{
    WebGlBuffer, WebGlFramebuffer, WebGlProgram, WebGlRenderingContext, WebGlShader, WebGlTexture,
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

struct Effect {
    context: Rc<WebGlRenderingContext>,
    shader_passes: Vec<Shader>,
}

pub struct Model {
    context: Rc<WebGlRenderingContext>,
    effect_chain: Vec<Effect>,
    blit_shader: Shader,
    blank_texture: WebGlTexture,
    extra_fbo: RefCell<Fbo>,
    square_vertex_buffer: WebGlBuffer,
}

impl Effect {
    fn new(context: Rc<WebGlRenderingContext>) -> Effect {
        let mut shader_passes = Vec::new();

        shader_passes.push(Shader::from_fragment_shader(
            Rc::clone(&context),
            r#"
            precision mediump float;
            varying highp vec2 uv;

            void main() {
                vec2 normCoord = 2. * (uv - 0.5);
                gl_FragColor = vec4(abs(normCoord), 0., 1.);
            }
        "#,
        ));

        shader_passes.push(Shader::from_fragment_shader(Rc::clone(&context), r#"
            varying highp vec2 uv;
            uniform sampler2D iInput;
            precision mediump float;

            vec4 composite(vec4 under, vec4 over) {
                float a_out = 1. - (1. - over.a) * (1. - under.a);
                return clamp(vec4((over.rgb + under.rgb * (1. - over.a)), a_out), vec4(0.), vec4(1.));
            }

            void main() {
                float iIntensity = 0.8;
                vec4 c;
                vec2 normCoord = 2. * (uv - 0.5);

                c = vec4(1.) * (1. - smoothstep(iIntensity - 0.1, iIntensity, length(normCoord)));
                gl_FragColor = c;

                c = texture2D(iInput, (uv - 0.5) / iIntensity + 0.5);
                c *= 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(normCoord));
                gl_FragColor = composite(gl_FragColor, c);
                //gl_FragColor.rgb *= gl_FragColor.a;
                //gl_FragColor.a = 1.0;
            }

        "#));

        Effect {
            context,
            shader_passes,
        }
    }

    fn paint<'a>(&'a self, model: &'a Model) -> Option<&'a RefCell<Fbo>> {
        let mut last_fbo: Option<&'a RefCell<Fbo>> = None;

        for shader in &self.shader_passes {
            model.bind_fbo_to_texture(GL::TEXTURE0, last_fbo);
            shader.paint(model, Some(&model.extra_fbo.borrow()));

            model.extra_fbo.swap(&shader.fbo);
            last_fbo = Some(&shader.fbo);
        }

        last_fbo
    }
}

impl Shader {
    fn from_fragment_shader(context: Rc<WebGlRenderingContext>, shader_code: &str) -> Shader {
        info!("Compiling shaders");
        let vertex_shader = Self::compile_shader(
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

    fn paint(&self, model: &Model, fbo: Option<&Fbo>) -> () {
        self.context
            .bind_framebuffer(GL::FRAMEBUFFER, fbo.map(|f| &f.framebuffer));

        self.context.use_program(Some(&self.program));

        let position_loc = self.context.get_attrib_location(&self.program, "vPosition");

        self.context
            .bind_buffer(GL::ARRAY_BUFFER, Some(&model.square_vertex_buffer));
        self.context.enable_vertex_attrib_array(position_loc as u32);
        self.context
            .vertex_attrib_pointer_with_i32(position_loc as u32, 4, GL::FLOAT, false, 0, 0);

        let texture_loc = self.context.get_uniform_location(&self.program, "iInput");
        self.context.uniform1i(texture_loc.as_ref(), 0);

        self.context.draw_arrays(GL::TRIANGLE_STRIP, 0, 4);
        self.context.use_program(None);
    }
}

impl Drop for Shader {
    fn drop(&mut self) -> () {
        self.context.delete_shader(Some(&self.fragment_shader));
        self.context.delete_shader(Some(&self.vertex_shader));
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
        self.context
            .bind_framebuffer(GL::FRAMEBUFFER, Some(&self.framebuffer));
    }
}

impl Drop for Fbo {
    fn drop(&mut self) -> () {
        self.context.delete_texture(Some(&self.texture));
        self.context.delete_framebuffer(Some(&self.framebuffer));
    }
}

impl Model {
    pub fn new(context: WebGlRenderingContext) -> Model {
        info!("New model & context!");
        let context = Rc::new(context);
        let extra_fbo = RefCell::new(Fbo::new(Rc::clone(&context)));
        let effect_chain = vec![Effect::new(Rc::clone(&context))];

        let blit_shader = Shader::from_fragment_shader(
            Rc::clone(&context),
            r#"
            varying highp vec2 uv;
            uniform sampler2D uSampler;
            void main() {
                gl_FragColor = texture2D(uSampler, uv);
            }
        "#,
        );

        let blank_texture = context.create_texture().unwrap();
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

        Model {
            context,
            effect_chain,
            extra_fbo,
            blit_shader,
            blank_texture,
            square_vertex_buffer,
        }
    }

    pub fn paint(&self) -> () {
        self.context.disable(GL::DEPTH_TEST);
        self.context.disable(GL::BLEND);
        self.context.clear_color(0.0, 0.0, 0.0, 0.0);
        self.context.clear(GL::COLOR_BUFFER_BIT);

        let mut last_fbo = None;
        for effect in &self.effect_chain {
            last_fbo = effect.paint(&self);
        }

        self.bind_fbo_to_texture(GL::TEXTURE0, last_fbo);
        self.blit_shader.paint(self, None);
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
}

impl Drop for Model {
    fn drop(&mut self) -> () {
        self.context.delete_buffer(Some(&self.square_vertex_buffer));
        self.context.delete_texture(Some(&self.blank_texture));
    }
}
