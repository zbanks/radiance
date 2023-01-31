use std::string::String;
use crate::context::{Context, ArcTextureViewSampler, RenderTargetState};
use crate::render_target::RenderTargetId;
use crate::CommonNodeProps;
use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use std::num::NonZeroU32;

const EFFECT_HEADER: &str = include_str!("effect_header.wgsl");
const EFFECT_FOOTER: &str = include_str!("effect_footer.wgsl");
const INTENSITY_INTEGRAL_PERIOD: f32 = 1024.;

/// Properties of an EffectNode.
/// Fields that are None are set to their default when the shader is loaded.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct EffectNodeProps {
    pub name: String,
    pub intensity: Option<f32>,
    pub frequency: Option<f32>,
    pub input_count: Option<u32>,
}

impl From<&EffectNodeProps> for CommonNodeProps {
    fn from(props: &EffectNodeProps) -> Self {
        CommonNodeProps {
            input_count: props.input_count,
        }
    }
}

#[allow(clippy::large_enum_variant)]
pub enum EffectNodeState {
    Uninitialized,
    Ready(EffectNodeStateReady),
    Error_(String), // ambiguous_associated_items error triggered by derive_more::TryInto without the _
}

pub struct EffectNodeStateReady {
    // Cached props
    name: String,
    intensity: f32,
    frequency: f32,
    input_count: u32,

    // Computed Info
    intensity_integral: f32,

    // GPU resources
    bind_group_layout: wgpu::BindGroupLayout,
    uniform_buffer: wgpu::Buffer,
    sampler: wgpu::Sampler,
    render_pipeline: wgpu::RenderPipeline,

    // Paint states
    paint_states: HashMap<RenderTargetId, EffectNodePaintState>,
}

struct EffectNodePaintState {
    output_texture: ArcTextureViewSampler,
}

// The uniform buffer associated with the effect (agnostic to render target)
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Uniforms {
    audio: [f32; 4],
    time: f32,
    frequency: f32,
    intensity: f32,
    intensity_integral: f32,
    resolution: [f32; 2],
    dt: f32,
    _padding: [u8; 4],
}

fn preprocess_shader(effect_source: &str) -> Result<(String, u32, f32), String> {
    let mut processed_source = String::new();
    let mut input_count = 1;
    let mut frequency = 0.;

    for l in effect_source.lines() {
        let line_parts: Vec<&str> = l.split_whitespace().collect();
        if !line_parts.is_empty() && line_parts[0] == "#property" {
            if line_parts.len() >= 2 {
                if line_parts[1] == "inputCount" {
                    if line_parts.len() >= 3 {
                        input_count = line_parts[2].parse::<u32>().map_err(|e| e.to_string())?;
                    } else {
                        return Err(String::from("inputCount missing argument"));
                    }
                } else if line_parts[1] == "frequency" {
                    if line_parts.len() >= 3 {
                        frequency = line_parts[2].parse::<f32>().map_err(|e| e.to_string())?;
                    } else {
                        return Err(String::from("frequency missing argument"));
                    }
                } else {
                    return Err(format!("Unrecognized property: {}", line_parts[1]));
                }
            } else {
                return Err(String::from("Missing property name"));
            }
        } else {
            processed_source.push_str(l);
            processed_source.push('\n');
        }
    }

    Ok((processed_source, input_count, frequency))
}

// This is a state machine, it's more natural to use `match` than `if let`
#[allow(clippy::single_match)]
impl EffectNodeState {
    fn setup_render_pipeline(ctx: &Context, props: &EffectNodeProps) -> Result<EffectNodeStateReady, String> {
        let name = &props.name;

        // Shader
        let effect_source = ctx.fetch_content(name).expect("Failed to read effect shader file");

        let (effect_source_processed, shader_input_count, default_frequency) = preprocess_shader(&effect_source)?;

        let input_count = match props.input_count {
            Some(input_count) => {
                if shader_input_count != input_count {
                    return Err("Shader input count does not match input count declared in graph".to_string());
                }
                input_count
            },
            None => {
                shader_input_count
            },
        };

        // Default to 0 intensity if none given
        let intensity = props.intensity.unwrap_or(0.);
        let frequency = props.frequency.unwrap_or(default_frequency);

        let shader_source = &format!("{}\n{}\n{}\n", EFFECT_HEADER, effect_source_processed, EFFECT_FOOTER);
        let shader_module = ctx.device().create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(&format!("EffectNode {} shader", name)),
            source: wgpu::ShaderSource::Wgsl(shader_source.into()),
        });

        // The uniforms bind group:
        // 0: Uniforms
        // 1: iSampler
        // 2: iInputsTex[]
        // 3: iNoiseTex
        // 4: iChannelTex[]

        let bind_group_layout = ctx.device().create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0, // UpdateUniforms
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1, // iSampler
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 2, // iInputsTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    },
                    count: NonZeroU32::new(input_count),
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 3, // iNoiseTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    },
                    count: None,
                },
                //wgpu::BindGroupLayoutEntry {
                //    binding: 4, // iChannelTex
                //    visibility: wgpu::ShaderStages::FRAGMENT,
                //    ty: wgpu::BindingType::Texture {
                //        multisampled: false,
                //        view_dimension: wgpu::TextureViewDimension::D2,
                //        sample_type: wgpu::TextureSampleType::Uint,
                //    },
                //    count: NonZeroU32::new(input_count),
                //},
            ],
            label: Some(&format!("EffectNode {} bind group layout", name)),
        });

        let render_pipeline_layout =
            ctx.device().create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Render Pipeline Layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            });

        // Create a render pipeline, we will eventually want multiple of these for a multi-pass effect
        let render_pipeline = ctx.device().create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some(&format!("EffectNode {} render pipeline", name)),
            layout: Some(&render_pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader_module,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader_module,
                entry_point: "fs_main",
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    blend: None,
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleStrip,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: Some(wgpu::Face::Back),
                polygon_mode: wgpu::PolygonMode::Fill,
                unclipped_depth: false,
                conservative: false,
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });

        // The update uniform buffer for this effect
        let uniform_buffer = ctx.device().create_buffer(
            &wgpu::BufferDescriptor {
                label: Some(&format!("EffectNode {} uniform buffer", name)),
                size: std::mem::size_of::<Uniforms>() as u64,
                usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            }
        );

        // The sampler that will be used for texture access within the shaders
        let sampler = ctx.device().create_sampler(
            &wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Linear,
                min_filter: wgpu::FilterMode::Linear,
                mipmap_filter: wgpu::FilterMode::Linear,
                ..Default::default()
            }
        );

        Ok(EffectNodeStateReady {
            name: name.clone(),
            intensity,
            frequency,
            input_count,
            intensity_integral: 0.,
            bind_group_layout,
            uniform_buffer,
            sampler,
            render_pipeline,
            paint_states: HashMap::new(),
        })
    }

    fn new_paint_state(_self_ready: &EffectNodeStateReady, ctx: &Context, render_target_state: &RenderTargetState) -> EffectNodePaintState {
        let texture_desc = wgpu::TextureDescriptor {
            size: wgpu::Extent3d {
                width: render_target_state.width(),
                height: render_target_state.height(),
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::COPY_SRC
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::TEXTURE_BINDING
                ,
            label: None,
        };

        let texture = ctx.device().create_texture(&texture_desc);
        let view = texture.create_view(&Default::default());
        let sampler = ctx.device().create_sampler(
            &wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Linear,
                min_filter: wgpu::FilterMode::Linear,
                mipmap_filter: wgpu::FilterMode::Linear,
                ..Default::default()
            }
        );

        EffectNodePaintState{
            output_texture: ArcTextureViewSampler::new(texture, view, sampler),
        }
    }

    fn update_paint_states(self_ready: &mut EffectNodeStateReady, ctx: &Context) {
        // See if we need to add or remove any paint states
        // (based on the context's render targets)

        self_ready.paint_states.retain(|id, _| ctx.render_target_states().contains_key(id));

        for (check_render_target_id, render_target_state) in ctx.render_target_states().iter() {
            if !self_ready.paint_states.contains_key(check_render_target_id) {
                self_ready.paint_states.insert(*check_render_target_id, Self::new_paint_state(self_ready, ctx, render_target_state));
            }
        }
    }

    pub fn new(ctx: &Context, props: &EffectNodeProps) -> Self {
        // TODO kick of shader compilation in the background instead of blocking
        match Self::setup_render_pipeline(ctx, props) {
            Ok(mut new_obj_ready) => {
                Self::update_paint_states(&mut new_obj_ready, ctx);
                Self::Ready(new_obj_ready)
            },
            Err(msg) => {
                Self::Error_(msg)
            }
        }
    }

    pub fn update(&mut self, ctx: &Context, props: &mut EffectNodeProps) {
        match self {
            EffectNodeState::Ready(self_ready) => {
                if props.name != self_ready.name {
                    *self = EffectNodeState::Error_("EffectNode name changed after construction".to_string());
                    return;
                }
                match props.input_count {
                    Some(input_count) => {
                        // Caller passed in an input_count, we should validate it
                        if input_count != self_ready.input_count {
                            *self = EffectNodeState::Error_("EffectNode input_count changed after construction".to_string());
                            return;
                        }
                    },
                    _ => {},
                }
                match props.intensity {
                    Some(intensity) => {
                        // Cache the intensity for when paint() is called
                        self_ready.intensity = intensity;
                    },
                    _ => {},
                }
                match props.frequency {
                    Some(frequency) => {
                        // Cache the frequency for when paint() is called
                        self_ready.frequency = frequency;
                    },
                    _ => {},
                }

                // Report back to the caller what our props are
                self_ready.update_props(props);

                Self::update_paint_states(self_ready, ctx);

                // Accumulate intensity_integral
                self_ready.intensity_integral = (self_ready.intensity_integral + self_ready.intensity * ctx.dt) % INTENSITY_INTEGRAL_PERIOD;
            },
            _ => {}
        }
    }

    pub fn paint(&mut self, ctx: &Context, encoder: &mut wgpu::CommandEncoder, render_target_id: RenderTargetId, inputs: &[Option<ArcTextureViewSampler>]) -> ArcTextureViewSampler {

        match self {
            EffectNodeState::Ready(self_ready) => {
                let render_target_state = ctx.render_target_state(render_target_id).expect("Call to paint() with a render target ID unknown to the context");
                let paint_state = self_ready.paint_states.get(&render_target_id).expect("Call to paint() with a render target ID unknown to the node (did you call update() first?)");

                // Populate the uniforms
                {
                    let width = render_target_state.width();
                    let height = render_target_state.height();
                    let uniforms = Uniforms {
                        audio: [0., 0., 0., 0.],
                        time: ctx.time,
                        frequency: self_ready.frequency,
                        intensity: self_ready.intensity,
                        intensity_integral: self_ready.intensity_integral,
                        resolution: [width as f32, height as f32],
                        dt: render_target_state.dt(),
                        ..Default::default()
                    };
                    ctx.queue().write_buffer(&self_ready.uniform_buffer, 0, bytemuck::cast_slice(&[uniforms]));
                }

                // Make an array of input textures
                let input_binding: Vec<&wgpu::TextureView> = (0..self_ready.input_count).map(|i| {
                    match inputs.get(i as usize) {
                        Some(Some(tex)) => tex.view.as_ref(),
                        _ => ctx.blank_texture().view.as_ref(),
                    }
                }).collect();

                let bind_group = ctx.device().create_bind_group(&wgpu::BindGroupDescriptor {
                    layout: &self_ready.bind_group_layout,
                    entries: &[
                        wgpu::BindGroupEntry {
                            binding: 0,
                            resource: self_ready.uniform_buffer.as_entire_binding(),
                        },
                        wgpu::BindGroupEntry {
                            binding: 1,
                            resource: wgpu::BindingResource::Sampler(&self_ready.sampler),
                        },
                        wgpu::BindGroupEntry {
                            binding: 2, // iInputsTex
                            resource: wgpu::BindingResource::TextureViewArray(input_binding.as_slice())
                        },
                        wgpu::BindGroupEntry {
                            binding: 3, // iNoiseTex
                            resource: wgpu::BindingResource::TextureView(&render_target_state.noise_texture().view)
                        },
                        //wgpu::BindGroupEntry {
                        //    binding: 4, // iChannelTex
                        //    resource: wgpu::BindingResource::TextureViewArray()
                        //},
                    ],
                    label: Some("EffectNode bind group"),
                });

                {
                    let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: Some("EffectNode render pass"),
                        color_attachments: &[
                            Some(wgpu::RenderPassColorAttachment {
                                view: &paint_state.output_texture.view,
                                resolve_target: None,
                                ops: wgpu::Operations {
                                    load: wgpu::LoadOp::Clear(
                                        wgpu::Color {
                                            r: 0.9,
                                            g: 0.2,
                                            b: 0.3,
                                            a: 1.0,
                                        }
                                    ),
                                    store: true,
                                }
                            }),
                        ],
                        depth_stencil_attachment: None,
                    });

                    render_pass.set_pipeline(&self_ready.render_pipeline);
                    render_pass.set_bind_group(0, &bind_group, &[]); 
                    render_pass.draw(0..4, 0..1);
                }

                paint_state.output_texture.clone()
            },
            _ => ctx.blank_texture().clone(),
        }
    }
}

impl EffectNodeStateReady {
    fn update_props(&self, props: &mut EffectNodeProps) {
        props.name.clone_from(&self.name);
        props.intensity = Some(self.intensity);
        props.frequency = Some(self.frequency);
        props.input_count= Some(self.input_count);
    }
}

//impl EffectNodePaintState {
//    pub fn new(width: u32, height: u32) -> EffectNodePaintState {
//        let texture_desc = wgpu::TextureDescriptor {
//            size: wgpu::Extent3d {
//                width,
//                height,
//                depth_or_array_layers: 1,
//            },
//            mip_level_count: 1,
//            sample_count: 1,
//            view_dimension: wgpu::TextureDimension::D2,
//            format: wgpu::TextureFormat::Rgba8UnormSrgb,
//            usage: wgpu::TextureUsages::COPY_SRC
//                | wgpu::TextureUsages::RENDER_ATTACHMENT
//                ,
//            label: None,
//        };
//        let texture = self.device.create_texture(&texture_desc);
//
//        EffectNodePaintState {
//            width,
//            height,
//            texture,
//        }
//    }
//
//}
