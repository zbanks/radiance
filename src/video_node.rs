use crate::err::Result;
use crate::graphics::{Fbo, RenderChain, Shader};
use crate::resources;

use log::*;
use std::cell::RefCell;
use std::collections::HashMap;
use web_sys::WebGlRenderingContext as GL;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct VideoNodeId {
    id: usize,
}

pub struct VideoNode {
    pub id: VideoNodeId,
    pub name: String,
    pub n_inputs: usize,
    pub kind: Box<dyn VideoNodeKind>,
    time: f64,
}

pub trait VideoNodeKind {
    fn artist(&self, chain: &RenderChain) -> Result<Box<dyn VideoArtist>>;
    fn set_time(&mut self, _time: f64, _dt: f64) {}

    // These should not be part of the trait
    fn set_intensity(&mut self, _intensity: f64) {}
    fn intensity(&self) -> Option<f64> {
        None
    }
}

pub trait VideoArtist {
    fn render<'a>(
        &'a self,
        chain: &'a RenderChain,
        node: &'a VideoNode,
        input_fbos: &[Option<&'a RefCell<Fbo>>],
    );
    fn fbo<'a>(&'a self) -> Option<&'a RefCell<Fbo>>;
}

struct EffectNode {
    intensity: f64,
    intensity_integral: f64,
    shader_sources: Vec<String>,
    properties: HashMap<String, String>,
}

struct OutputNode {}

struct EffectArtist {
    shader_passes: Vec<Shader>,
}

struct OutputArtist {
    blit_shader: Shader,
}

impl VideoNodeId {
    fn new() -> VideoNodeId {
        let id = unsafe {
            static mut NEXT_ID: usize = 0;
            NEXT_ID += 1;
            NEXT_ID
        };
        VideoNodeId { id }
    }
}

impl VideoNode {
    pub fn output() -> Result<VideoNode> {
        let id = VideoNodeId::new();
        Ok(VideoNode {
            id,
            name: String::from("Output"),
            n_inputs: 1,
            time: 0.0,
            kind: Box::new(OutputNode {}),
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

        let id = VideoNodeId::new();
        Ok(VideoNode {
            id,
            name: String::from(name),
            n_inputs,
            time: 0.0,
            kind: Box::new(EffectNode {
                intensity: 0.0,
                intensity_integral: 0.0,
                shader_sources,
                properties,
            }),
        })
    }

    pub fn set_time(&mut self, time: f64) {
        let dt = time - self.time;
        self.kind.set_time(time, dt);
        self.time = time;
    }

    pub fn id(&self) -> VideoNodeId {
        self.id
    }
}

impl VideoNodeKind for EffectNode {
    fn set_time(&mut self, _time: f64, dt: f64) {
        self.intensity_integral = (self.intensity_integral + dt * self.intensity) % 1024.0;
    }

    fn set_intensity(&mut self, intensity: f64) {
        self.intensity = intensity;
    }

    fn intensity(&self) -> Option<f64> {
        Some(self.intensity)
    }

    fn artist(&self, chain: &RenderChain) -> Result<Box<dyn VideoArtist>> {
        let shader_passes = self
            .shader_sources
            .iter()
            .map(|s| chain.compile_fragment_shader(&s))
            .collect::<Result<_>>()?;
        Ok(Box::new(EffectArtist { shader_passes }))
    }
}

impl VideoNodeKind for OutputNode {
    fn artist(&self, chain: &RenderChain) -> Result<Box<dyn VideoArtist>> {
        let blit_shader = chain.compile_fragment_shader(resources::glsl::PLAIN_FRAGMENT)?;
        Ok(Box::new(OutputArtist { blit_shader }))
    }
}

impl VideoArtist for EffectArtist {
    fn render<'a>(
        &'a self,
        chain: &'a RenderChain,
        node: &'a VideoNode,
        input_fbos: &[Option<&'a RefCell<Fbo>>],
    ) {
        //let effect_node = node.kind.dyn_cast::<EffectNode>().unwrap();
        //let effect_node = node.kind.downcast::<EffectNode>().unwrap();
        let intensity = node.kind.intensity().unwrap();
        let intensity_integral = node.kind.intensity().unwrap(); // XXX fixme
                                                                 //let intensity_integral = effect_node.intensity_integral;
        for shader in self.shader_passes.iter().rev() {
            let active_shader = shader.begin_render(chain, Some(&chain.extra_fbo.borrow()));
            let mut tex_index: u32 = 0;

            chain.context.active_texture(GL::TEXTURE0 + tex_index);
            chain
                .context
                .bind_texture(GL::TEXTURE_2D, Some(&chain.noise_texture));
            let loc = active_shader.get_uniform_location("iNoise");
            chain.context.uniform1i(loc.as_ref(), tex_index as i32);
            tex_index += 1;

            let mut inputs: Vec<i32> = vec![];
            for fbo in input_fbos {
                chain.bind_fbo_to_texture(GL::TEXTURE0 + tex_index, *fbo);
                inputs.push(tex_index as i32);
                tex_index += 1;
            }
            let loc = active_shader.get_uniform_location("iInputs");
            chain
                .context
                .uniform1iv_with_i32_array(loc.as_ref(), &inputs);

            let mut channels: Vec<i32> = vec![];
            for shader in &self.shader_passes {
                chain.bind_fbo_to_texture(GL::TEXTURE0 + tex_index, Some(&shader.fbo));
                channels.push(tex_index as i32);
                tex_index += 1;
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

    fn fbo<'a>(&'a self) -> Option<&'a RefCell<Fbo>> {
        Some(&self.shader_passes.first()?.fbo)
    }
}

impl VideoArtist for OutputArtist {
    fn render<'a>(
        &'a self,
        chain: &'a RenderChain,
        _node: &'a VideoNode,
        input_fbos: &[Option<&'a RefCell<Fbo>>],
    ) {
        // TODO: An unfortunate part about this implementation is that the Output kind
        // requires an extra Shader (with associated FBO) & blit operation
        // Ideally this artist could just return its first input (this is how it was
        // implemented in C++) -- however, it's hard to make the lifetime of the input
        // sufficient. From a high level, it's hard to guarantee that the input FBO still
        // holds the same contents when `artist.fbo()` is called as when
        // `artist.render(...)` was originally called.
        let active_shader = self
            .blit_shader
            .begin_render(chain, Some(&chain.extra_fbo.borrow()));

        chain.bind_fbo_to_texture(GL::TEXTURE0, input_fbos[0]);
        let loc = active_shader.get_uniform_location("iInputs");
        chain.context.uniform1iv_with_i32_array(loc.as_ref(), &[0]);

        active_shader.finish_render();
        chain.extra_fbo.swap(&self.blit_shader.fbo);
    }
    fn fbo<'a>(&'a self) -> Option<&'a RefCell<Fbo>> {
        Some(&self.blit_shader.fbo)
    }
}
