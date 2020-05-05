use crate::err::Result;
use crate::graphics::{Fbo, RenderChain, Shader};
use crate::resources;
use crate::video_node::{DetailLevel, IVideoNode, VideoNodeDiscriminants, VideoNodeId};

use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;
use std::borrow::Borrow;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use strum_macros::EnumString;
use web_sys::WebGlRenderingContext as GL;

#[serde(rename_all = "camelCase")]
#[derive(Serialize)]
pub struct EffectNode {
    #[serde(rename = "uid")]
    id: VideoNodeId,
    node_type: VideoNodeDiscriminants,
    dirty: RefCell<Option<DetailLevel>>,

    name: String,
    n_inputs: usize,
    header: EffectHeader,
    properties: HashMap<String, String>,

    time: f64,
    intensity: f64,
    frequency: f64,

    #[serde(skip)]
    intensity_integral: f64,
    #[serde(skip)]
    shader_sources: Vec<String>,
    #[serde(skip)]
    shader_passes: Vec<Option<Shader>>,
}

#[strum(serialize_all = "snake_case")]
#[serde(rename_all = "snake_case")]
#[derive(EnumString, Serialize)]
enum EffectHeader {
    /// Default WebGL Radiance
    Radiance17,
    // /// Matches Qt Radiance v0.5.0, adds frequency
    // Radiance18,
    // /// ShaderToy
    // Shadertoy,
}

#[serde(rename_all = "camelCase")]
#[derive(Debug, Deserialize)]
struct LocalState {
    name: Option<String>,
    intensity: Option<f64>,
    frequency: Option<f64>,
}

impl Default for EffectHeader {
    fn default() -> EffectHeader {
        EffectHeader::Radiance17
    }
}

impl EffectNode {
    pub fn new() -> Result<EffectNode> {
        let id = VideoNodeId::new();
        Ok(EffectNode {
            id,
            node_type: VideoNodeDiscriminants::EffectNode,
            dirty: RefCell::new(None),

            name: String::from(""),
            n_inputs: 1,
            header: Default::default(),
            properties: Default::default(),

            time: 0.0,
            intensity: 0.0,
            frequency: 0.0,

            intensity_integral: 0.0,
            shader_sources: Default::default(),
            shader_passes: Default::default(),
        })
    }

    /// Change the EffectNode's name, which also recompiles the shader source
    fn set_name(&mut self, name: &str) -> Result<()> {
        if name == self.name.as_str() {
            return Ok(());
        }

        let program = resources::effects::lookup(name).ok_or("Unknown effect name")?;

        let mut properties = HashMap::new();
        let mut shader_sources: Vec<String> = Vec::new();
        let mut source = String::new();
        source.push_str("\n#line 1\n");

        for (i, line) in program.split('\n').enumerate() {
            let mut terms = line.trim().splitn(3, ' ');
            let head = terms.next();
            match head {
                // Syntax: `#property <key> <value>`
                // Inserts `(key, value)` into the properties map
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
                // Syntax: `#buffershader`
                // Separates shaders into multiple passses
                Some("#buffershader") => {
                    shader_sources.push(source);
                    source = String::new();
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

        let header: EffectHeader = properties
            .get("header")
            .map_or(Ok(Default::default()), |x| {
                x.parse().map_err(|_| "Invalid header")
            })?;

        // Prepend the appropriate header to every shader source
        let header_source = match header {
            EffectHeader::Radiance17 => String::from(resources::glsl::EFFECT_HEADER),
        };
        for src in shader_sources.iter_mut() {
            *src = format!("{}{}", header_source, src);
        }

        // Create an empty Vec to hold shader_passes; these are compiled in pre_render
        let shader_passes = shader_sources.iter().map(|_| None).collect();

        self.name = name.to_string();
        self.n_inputs = n_inputs;
        self.header = header;
        self.properties = properties;
        self.shader_sources = shader_sources;
        self.shader_passes = shader_passes;

        Ok(())
    }
}

impl IVideoNode for EffectNode {
    fn id(&self) -> VideoNodeId {
        self.id
    }

    fn name(&self) -> &str {
        &self.name
    }

    fn n_inputs(&self) -> usize {
        self.n_inputs
    }

    fn n_buffers(&self) -> usize {
        self.shader_passes.len()
    }

    fn pre_render(&mut self, chain: &RenderChain, time: f64) {
        let dt = time - self.time;
        self.intensity_integral = (self.intensity_integral + dt * self.intensity) % 1024.0;
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
            let sample = chain.audio();
            let audio: [f32; 4] = [sample.low, sample.mid, sample.high, sample.level];
            active_shader.set_uniform1iv("iChannel", &channels);
            active_shader.set_uniform1f("iIntensity", self.intensity as f32);
            active_shader.set_uniform1f("iIntensityIntegral", self.intensity_integral as f32);
            active_shader.set_uniform1f("iTime", (self.time % 2048.) as f32);
            active_shader.set_uniform1f("iStep", (self.time % 2048.) as f32);
            active_shader.set_uniform1f("iFPS", 60.);
            active_shader.set_uniform4fv("iAudio", &audio);
            active_shader.set_uniform2f("iResolution", (chain.size.0 as f32, chain.size.1 as f32));
            //web_sys::console::time_with_label("render_finish");
            active_shader.finish_render();
            //web_sys::console::time_end_with_label("render_finish");

            std::mem::swap(&mut *chain.extra_fbo.borrow_mut(), &mut buffer_fbos[i]);
        }
        buffer_fbos.first().map(|fbo| Rc::clone(&fbo))
    }

    fn state(&self, _level: DetailLevel) -> JsonValue {
        serde_json::to_value(&self).unwrap_or(JsonValue::Null)
    }

    fn set_state(&mut self, raw_state: JsonValue) -> Result<()> {
        let state: LocalState = serde_json::from_value(raw_state)?;
        if let Some(name) = state.name {
            self.set_name(&name)?;
        }
        if let Some(intensity) = state.intensity {
            self.intensity = intensity;
        }
        if let Some(frequency) = state.frequency {
            self.frequency = frequency;
        }
        *self.dirty.borrow_mut() = Some(DetailLevel::All);
        Ok(())
    }

    fn flush(&self) -> Option<DetailLevel> {
        let dirty = RefCell::new(None);
        self.dirty.swap(&dirty);
        dirty.into_inner()
    }
}
