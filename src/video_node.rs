use crate::err::Result;
use crate::graphics::{Fbo, RenderChain, Shader};
use crate::resources;
use log::*;
use std::cell::RefCell;
use std::collections::HashMap;
use web_sys::WebGlRenderingContext as GL;

pub struct VideoNode {
    pub id: usize,
    pub name: String,
    pub n_inputs: usize,
    pub kind: VideoNodeKind,
    time: f64,
}

#[derive(Debug)]
pub enum VideoNodeKind {
    Effect {
        intensity: f64,
        intensity_integral: f64,
        shader_sources: Vec<String>,
        properties: HashMap<String, String>,
    },
    Output,
}

pub enum VideoArtist {
    Effect { shader_passes: Vec<Shader> },
    Output { blit_shader: Shader },
}

impl VideoNode {
    fn generate_id() -> usize {
        unsafe {
            static mut NEXT_ID: usize = 0;
            NEXT_ID += 1;
            NEXT_ID
        }
    }

    pub fn output() -> Result<VideoNode> {
        let id = Self::generate_id();
        Ok(VideoNode {
            id,
            name: String::from("Output"),
            n_inputs: 1,
            time: 0.0,
            kind: VideoNodeKind::Output,
        })
    }

    pub fn effect(name: &str) -> Result<VideoNode> {
        let program = resources::effects::lookup(name).ok_or("Unknown effect name")?;

        let header_source = String::from(resources::glsl::EFFECT_HEADER);
        let mut source = String::new();
        let mut properties = HashMap::new();

        source.push_str(&header_source);
        source.push_str("\n#line 1\n");

        let mut shader_sources = Vec::new();
        for (i, line) in program.split('\n').enumerate() {
            let mut terms = line.trim().splitn(3, ' ');
            let head = terms.next();
            match head {
                Some("#property") => {
                    let key = terms
                        .next()
                        .ok_or("Parse error in #property line")?
                        .to_string();
                    let value = terms
                        .next()
                        .ok_or("Parse error in #property line")?
                        .to_string();
                    properties.insert(key, value);
                }
                Some("#buffershader") => {
                    shader_sources.push(source);
                    source = String::new();
                    source.push_str(&header_source);
                    source.push_str(&format!("\n#line {}\n", i + 1));
                }
                _ => {
                    source.push_str(&line);
                }
            }
            source.push_str("\n");
        }
        shader_sources.push(source);

        let n_inputs: usize = properties
            .get("inputCount")
            .map_or(Ok(1), |x| x.parse().map_err(|_| "Invalid inputCount"))?;

        info!("Loaded effect: {:?}", name);

        let id = Self::generate_id();
        Ok(VideoNode {
            id,
            name: String::from(name),
            n_inputs,
            time: 0.0,
            kind: VideoNodeKind::Effect {
                intensity: 0.0,
                intensity_integral: 0.0,
                shader_sources,
                properties,
            },
        })
    }

    pub fn set_time(&mut self, time: f64) {
        let dt = time - self.time;
        match self.kind {
            VideoNodeKind::Effect {
                intensity,
                ref mut intensity_integral,
                ..
            } => {
                *intensity_integral = (*intensity_integral + dt * intensity) % 1024.0;
            }
            VideoNodeKind::Output => (),
        }
        //self.intensity_integral = (self.intensity_integral + dt * self.intensity) % 1024.0;
        self.time = time;
    }

    pub fn id(&self) -> usize {
        self.id
    }

    pub fn artist(&self, chain: &RenderChain) -> Result<VideoArtist> {
        info!("New artist! {:?}", self.kind);
        match self.kind {
            VideoNodeKind::Effect {
                ref shader_sources, ..
            } => {
                let shader_passes = shader_sources
                    .iter()
                    .map(|s| chain.compile_fragment_shader(&s))
                    .collect::<Result<_>>()?;
                Ok(VideoArtist::Effect { shader_passes })
            }
            VideoNodeKind::Output => {
                let blit_shader = chain.compile_fragment_shader(resources::glsl::PLAIN_FRAGMENT)?;
                Ok(VideoArtist::Output { blit_shader })
            }
        }
    }
}

impl VideoArtist {
    pub fn render<'a>(
        &'a self,
        chain: &'a RenderChain,
        node: &'a VideoNode,
        //input_fbos: Option<&'a RefCell<Fbo>>,
        input_fbos: &[Option<&'a RefCell<Fbo>>],
    ) {
        match self {
            VideoArtist::Effect { shader_passes } => {
                let (intensity, intensity_integral) = match node.kind {
                    VideoNodeKind::Effect {
                        intensity,
                        intensity_integral,
                        ..
                    } => (intensity, intensity_integral),
                    _ => (0.0, 0.0),
                };
                for shader in shader_passes.iter().rev() {
                    let active_shader = shader.begin_render(chain, Some(&chain.extra_fbo.borrow()));

                    let mut inputs: Vec<i32> = vec![];
                    for (i, fbo) in input_fbos.iter().enumerate() {
                        chain.bind_fbo_to_texture(GL::TEXTURE0 + i as u32, *fbo);
                        inputs.push(i as i32);
                    }
                    let loc = active_shader.get_uniform_location("iInputs");
                    chain
                        .context
                        .uniform1iv_with_i32_array(loc.as_ref(), &inputs);

                    let mut channels: Vec<i32> = vec![];
                    for (i, shader) in shader_passes.iter().enumerate() {
                        chain.bind_fbo_to_texture(
                            GL::TEXTURE0 + (inputs.len() + i) as u32,
                            Some(&shader.fbo),
                        );
                        channels.push((inputs.len() + i) as i32);
                    }
                    let loc = active_shader.get_uniform_location("iChannel");
                    chain
                        .context
                        .uniform1iv_with_i32_array(loc.as_ref(), &channels);

                    let loc = active_shader.get_uniform_location("iIntensity");
                    chain.context.uniform1f(loc.as_ref(), intensity as f32);

                    let loc = active_shader.get_uniform_location("iIntensityIntegral");
                    chain
                        .context
                        .uniform1f(loc.as_ref(), intensity_integral as f32);

                    let loc = active_shader.get_uniform_location("iTime");
                    chain
                        .context
                        .uniform1f(loc.as_ref(), (node.time % 2048.) as f32);

                    let loc = active_shader.get_uniform_location("iStep");
                    chain
                        .context
                        .uniform1f(loc.as_ref(), (node.time % 2048.) as f32);

                    let loc = active_shader.get_uniform_location("iFPS");
                    chain.context.uniform1f(loc.as_ref(), 60.);

                    let loc = active_shader.get_uniform_location("iAudio");
                    chain
                        .context
                        .uniform4fv_with_f32_array(loc.as_ref(), &[0.1, 0.2, 0.3, 0.4]);

                    let loc = active_shader.get_uniform_location("iResolution");
                    chain
                        .context
                        .uniform2f(loc.as_ref(), chain.size.0 as f32, chain.size.1 as f32);

                    active_shader.finish_render();
                    chain.extra_fbo.swap(&shader.fbo);
                }
            }
            VideoArtist::Output { blit_shader } => {
                // TODO: An unfortunate part about this implementation is that the Output kind
                // requires an extra Shader (with associated FBO) & blit operation
                // Ideally this artist could just return its first input (this is how it was
                // implemented in C++) -- however, it's hard to make the lifetime of the input
                // sufficient. From a high level, it's hard to guarantee that the input FBO still
                // holds the same contents when `artist.fbo()` is called as when
                // `artist.render(...)` was originally called.
                let active_shader =
                    blit_shader.begin_render(chain, Some(&chain.extra_fbo.borrow()));

                chain.bind_fbo_to_texture(GL::TEXTURE0, input_fbos[0]);
                let loc = active_shader.get_uniform_location("iInputs");
                chain.context.uniform1iv_with_i32_array(loc.as_ref(), &[0]);

                active_shader.finish_render();
                chain.extra_fbo.swap(&blit_shader.fbo);
            }
        };
    }

    pub fn fbo<'a>(&'a self) -> Option<&'a RefCell<Fbo>> {
        match self {
            VideoArtist::Effect { shader_passes } => Some(&shader_passes.first()?.fbo),
            VideoArtist::Output { blit_shader } => Some(&blit_shader.fbo),
        }
    }
}
