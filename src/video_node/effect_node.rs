use crate::err::Result;
use crate::graphics::{Fbo, RenderChain, Shader};
use crate::resources;
use crate::video_node::{VideoNode, VideoNodeId, VideoNodeKind, VideoNodeKindMut};

use log::*;
use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::borrow::Borrow;
use std::collections::HashMap;
use std::rc::Rc;
use web_sys::WebGlRenderingContext as GL;

pub struct EffectNode {
    state: State,
    time: f64,
    intensity_integral: f64,
    shader_sources: Vec<String>,
    shader_passes: Vec<Option<Shader>>,
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Serialize, Deserialize)]
struct State {
    #[serde(rename = "uid")]
    id: VideoNodeId,
    name: String,
    n_inputs: usize,
    #[serde(skip_deserializing)]
    properties: HashMap<String, String>,
    intensity: f64,
}

impl EffectNode {
    pub fn new(name: &str) -> Result<EffectNode> {
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

        let shader_passes = shader_sources.iter().map(|_| None).collect();

        let n_inputs: usize = properties
            .get("inputCount")
            .map_or(Ok(1), |x| x.parse().map_err(|_| "Invalid inputCount"))?;

        info!("Loaded effect: {:?}", name);

        let id = VideoNodeId::new();
        let state = State {
            id,
            name: name.to_string(),
            n_inputs,
            properties,
            intensity: 0.0,
        };
        Ok(EffectNode {
            state,
            time: 0.0,
            intensity_integral: 0.0,
            shader_sources,
            shader_passes,
        })
    }

    pub fn set_intensity(&mut self, intensity: f64) {
        self.state.intensity = intensity;
    }

    pub fn intensity(&self) -> f64 {
        self.state.intensity
    }
}

impl VideoNode for EffectNode {
    fn id(&self) -> VideoNodeId {
        self.state.id
    }

    fn name(&self) -> &str {
        &self.state.name
    }

    fn n_inputs(&self) -> usize {
        self.state.n_inputs
    }

    fn n_buffers(&self) -> usize {
        self.shader_passes.len()
    }

    fn pre_render(&mut self, chain: &RenderChain, time: f64) {
        let dt = time - self.time;
        self.intensity_integral = (self.intensity_integral + dt * self.intensity()) % 1024.0;
        self.time = time;

        for (src, shader) in self
            .shader_sources
            .iter()
            .zip(self.shader_passes.iter_mut())
        {
            if shader.is_none() {
                if let Ok(new_shader) = chain.compile_fragment_shader(src) {
                    shader.replace(new_shader);
                }
            }
        }
    }

    fn render<'a>(
        &'a self,
        chain: &'a RenderChain,
        input_fbos: &[Option<Rc<Fbo>>],
        buffer_fbos: &mut [Rc<Fbo>],
    ) -> Option<Rc<Fbo>> {
        assert!(input_fbos.len() == self.n_inputs());
        assert!(buffer_fbos.len() == self.n_buffers());
        for (i, shader) in self.shader_passes.iter().enumerate().rev() {
            let shader: &Shader = shader.as_ref().unwrap_or(&chain.blit_shader);

            let active_shader = shader.begin_render(chain, Some(&chain.extra_fbo.borrow()));
            let mut tex_index: u32 = 0;

            chain.context.active_texture(GL::TEXTURE0 + tex_index);
            chain
                .context
                .bind_texture(GL::TEXTURE_2D, Some(&chain.noise_texture));
            active_shader.set_uniform1i("iNoise", tex_index as i32);
            tex_index += 1;

            let mut inputs: Vec<i32> = vec![];
            for fbo in input_fbos {
                chain.bind_fbo_to_texture(
                    GL::TEXTURE0 + tex_index,
                    fbo.as_ref().map(|x| x.borrow()),
                );
                inputs.push(tex_index as i32);
                tex_index += 1;
            }
            active_shader.set_uniform1iv("iInputs", &inputs);

            let mut channels: Vec<i32> = vec![];
            for fbo in buffer_fbos.iter() {
                chain.bind_fbo_to_texture(GL::TEXTURE0 + tex_index, Some(fbo));
                channels.push(tex_index as i32);
                tex_index += 1;
            }
            active_shader.set_uniform1iv("iChannel", &channels);
            active_shader.set_uniform1f("iIntensity", self.intensity() as f32);
            active_shader.set_uniform1f("iIntensityIntegral", self.intensity_integral as f32);
            active_shader.set_uniform1f("iTime", (self.time % 2048.) as f32);
            active_shader.set_uniform1f("iStep", (self.time % 2048.) as f32);
            active_shader.set_uniform1f("iFPS", 60.);
            active_shader.set_uniform4fv("iAudio", &[0.1, 0.2, 0.3, 0.4]);
            active_shader.set_uniform2f("iResolution", (chain.size.0 as f32, chain.size.1 as f32));
            active_shader.finish_render();

            std::mem::swap(&mut *chain.extra_fbo.borrow_mut(), &mut buffer_fbos[i]);
        }
        Some(Rc::clone(&buffer_fbos.first().unwrap()))
    }

    fn state(&self) -> JsonValue {
        serde_json::to_value(&self.state).unwrap_or(JsonValue::Null)
    }

    fn set_state(&mut self, raw_state: JsonValue) -> Result<()> {
        let state: State = serde_json::from_value(raw_state)?;
        self.state.name = state.name;
        self.state.n_inputs = state.n_inputs;
        self.set_intensity(state.intensity);
        Ok(())
    }

    fn downcast(&self) -> Option<VideoNodeKind> {
        Some(VideoNodeKind::Effect(self))
    }
    fn downcast_mut(&mut self) -> Option<VideoNodeKindMut> {
        Some(VideoNodeKindMut::Effect(self))
    }
}
