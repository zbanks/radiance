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
    pub intensity: f64,
    time: f64,
    intensity_integral: f64,
    kind: VideoNodeKind,
}

#[derive(Debug)]
enum VideoNodeKind {
    Effect {
        shader_sources: Vec<String>,
        properties: HashMap<String, String>,
    },
}

pub enum VideoArtist {
    Effect { shader_passes: Vec<Shader> },
}

impl VideoNode {
    pub fn effect(name: &str) -> Result<VideoNode> {
        let program = resources::effects::lookup(name).ok_or("Unknown effect name")?;
        let id = unsafe {
            static mut NEXT_ID: usize = 0;
            NEXT_ID += 1;
            NEXT_ID
        };

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

        info!("Loaded effect: {:?}", properties);

        Ok(VideoNode {
            id,
            name: String::from(name),
            time: 0.0,
            intensity: 0.8,
            intensity_integral: 0.0,
            kind: VideoNodeKind::Effect {
                shader_sources,
                properties,
            },
        })
    }

    pub fn set_time(&mut self, time: f64) {
        let dt = time - self.time;
        self.intensity_integral = (self.intensity_integral + dt * self.intensity) % 1024.0;
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
                    chain.context.uniform1f(loc.as_ref(), node.intensity as f32);

                    let loc = active_shader.get_uniform_location("iIntensityIntegral");
                    chain
                        .context
                        .uniform1f(loc.as_ref(), node.intensity_integral as f32);

                    let loc = active_shader.get_uniform_location("iTime");
                    chain.context.uniform1f(loc.as_ref(), node.time as f32);

                    let loc = active_shader.get_uniform_location("iStep");
                    chain.context.uniform1f(loc.as_ref(), node.time as f32);

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
        };
    }

    pub fn fbo<'a>(&'a self) -> Option<&'a RefCell<Fbo>> {
        match self {
            VideoArtist::Effect { shader_passes } => Some(&shader_passes.first()?.fbo),
        }
    }
}
