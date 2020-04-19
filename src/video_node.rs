use crate::graphics::{ChainSize, Fbo, RenderChain, Shader};
use crate::resources;
use log::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use web_sys::WebGlRenderingContext as GL;

pub struct VideoNode {
    id: usize,
    intensity: f64,
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
    pub fn new_effect(program: &str) -> VideoNode {
        let id = unsafe {
            static mut NEXT_ID: usize = 0;
            NEXT_ID += 1;
            NEXT_ID
        };

        let mut header_source = String::from(resources::glsl::EFFECT_HEADER);
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
                    // XXX
                    let key = terms.next().unwrap().to_string();
                    let value = terms.next().unwrap().to_string();
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

        VideoNode {
            id,
            time: 0.0,
            intensity: 0.8,
            intensity_integral: 0.0,
            kind: VideoNodeKind::Effect {
                shader_sources,
                properties,
            },
        }
    }

    pub fn update_time(&mut self, time: f64) -> () {
        self.time = time;
    }

    pub fn id(&self) -> usize {
        self.id
    }

    pub fn new_artist(&self, chain: &RenderChain) -> VideoArtist {
        info!("New artist! {:?}", self.kind);
        match self.kind {
            VideoNodeKind::Effect {
                ref shader_sources,
                ref properties,
            } => {
                let shader_passes = shader_sources
                    .iter()
                    .map(|s| Shader::from_fragment_shader(Rc::clone(&chain.context), &s))
                    .collect();
                VideoArtist::Effect { shader_passes }
            }
        }
    }
}

impl VideoArtist {
    pub fn paint<'a>(
        &'a self,
        chain: &'a RenderChain,
        node: &'a VideoNode,
        input_fbos: Option<&'a RefCell<Fbo>>,
    ) -> Option<&'a RefCell<Fbo>> {
        let on_fbo = input_fbos;
        let mut last_fbo = on_fbo;

        match self {
            VideoArtist::Effect { shader_passes } => {
                for shader in shader_passes.iter().rev() {
                    {
                        let active_shader =
                            shader.begin_render(chain, Some(&chain.extra_fbo.borrow()));

                        chain.bind_fbo_to_texture(GL::TEXTURE0, on_fbo);
                        let loc = active_shader.get_uniform_location("iInputs");
                        chain.context.uniform1iv_with_i32_array(loc.as_ref(), &[0]);

                        let mut channels: Vec<i32> = vec![];
                        for (i, shader) in shader_passes.iter().enumerate() {
                            chain.bind_fbo_to_texture(
                                GL::TEXTURE0 + 1 + i as u32,
                                Some(&shader.fbo),
                            );
                            channels.push(1 + i as i32);
                        }
                        let loc = active_shader.get_uniform_location("iChannel");
                        chain
                            .context
                            .uniform1iv_with_i32_array(loc.as_ref(), &channels);

                        let loc = active_shader.get_uniform_location("iIntensity");
                        chain.context.uniform1f(loc.as_ref(), node.intensity as f32);

                        let loc = active_shader.get_uniform_location("iTime");
                        chain.context.uniform1f(loc.as_ref(), node.time as f32);

                        let loc = active_shader.get_uniform_location("iResolution");
                        chain.context.uniform2f(
                            loc.as_ref(),
                            chain.context.drawing_buffer_width() as f32,
                            chain.context.drawing_buffer_height() as f32,
                        );

                        active_shader.finish_render();
                    }

                    chain.extra_fbo.swap(&shader.fbo);
                    last_fbo = Some(&shader.fbo);
                }
            }
        };

        last_fbo
    }
}
