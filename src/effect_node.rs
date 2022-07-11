use std::string::String;
use crate::context::{Context, ArcTextureViewSampler, RenderTargetState};
use crate::render_target::RenderTargetId;
use std::collections::HashMap;

const EFFECT_HEADER: &str = include_str!("effect_header.wgsl");
const EFFECT_FOOTER: &str = include_str!("effect_footer.wgsl");
const INTENSITY_INTEGRAL_PERIOD: f32 = 1024.;

#[derive(Debug, Clone)]
pub struct EffectNodeProps {
    pub name: String,
    pub intensity: f32,
    pub frequency: f32,
}

pub enum EffectNodeState {
    Uninitialized,
    Ready(EffectNodeStateReady),
    Error(String),
}

pub struct EffectNodeStateReady {
    // Info
    name: String,
    n_inputs: u32,
    intensity_integral: f32,

    // GPU resources
    render_pipeline: wgpu::RenderPipeline,
    update_bind_group: wgpu::BindGroup,
    paint_bind_group_layout: wgpu::BindGroupLayout,
    update_uniform_buffer: wgpu::Buffer,
    paint_uniform_buffer: wgpu::Buffer,

    // Paint states
    paint_states: HashMap<RenderTargetId, EffectNodePaintState>,
}

struct EffectNodePaintState {
    input_textures: Vec<ArcTextureViewSampler>,
    output_texture: ArcTextureViewSampler,
}

// The uniform buffer associated with the effect (agnostic to render target)
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
#[allow(non_snake_case)]
struct UpdateUniforms {
    audio: [f32; 4],
    time: f32,
    frequency: f32,
    intensity: f32,
    intensity_integral: f32,
    _padding: [u8; 0],
}

// The uniform buffer associated with the effect (specific to render target)
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
#[allow(non_snake_case)]
struct PaintUniforms {
    resolution: [f32; 2],
    dt: f32,
    _padding: [u8; 4],
}

impl EffectNodeState {
    fn setup_render_pipeline(ctx: &Context, name: &str) -> EffectNodeStateReady {
        // Shader
        let effect_source = ctx.fetch_content(name).expect("Failed to read effect shader file");
        let shader_source = &format!("{}\n{}\n{}\n", EFFECT_HEADER, effect_source, EFFECT_FOOTER);
        let shader_module = ctx.device().create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(&format!("EffectNode {} shader", name)),
            source: wgpu::ShaderSource::Wgsl(shader_source.into()),
        });

        let n_inputs = 1_u32; // XXX read from file

        // The effect will have two bind groups, one which will be bound in update() (most uniforms & sampler)
        // and one which will be bound in paint() (a few uniforms & textures)

        // The "update" bind group:
        // 0: UpdateUniforms
        // 1: iSampler

        // The "paint" bind group:
        // 0: PaintUniforms
        // 1: iInputsTex[]
        // 2: iNoiseTex
        // 3: iChannelTex[]

        let update_bind_group_layout = ctx.device().create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
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
            ],
            label: Some(&format!("EffectNode {} update bind group layout", name)),
        });

        let paint_bind_group_layout = ctx.device().create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0, // PaintUniforms
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
                //wgpu::BindGroupLayoutEntry {
                //    binding: 1, // iInputsTex
                //    visibility: wgpu::ShaderStages::FRAGMENT,
                //    ty: wgpu::BindingType::Texture {
                //        multisampled: false,
                //        view_dimension: wgpu::TextureViewDimension::D2,
                //        sample_type: wgpu::TextureSampleType::Uint,
                //    },
                //    count: NonZeroU32::new(n_inputs),
                //},
                //wgpu::BindGroupLayoutEntry {
                //    binding: 2, // iNoiseTex
                //    visibility: wgpu::ShaderStages::FRAGMENT,
                //    ty: wgpu::BindingType::Texture {
                //        multisampled: false,
                //        view_dimension: wgpu::TextureViewDimension::D2,
                //        sample_type: wgpu::TextureSampleType::Uint,
                //    },
                //    count: None,
                //},
                //wgpu::BindGroupLayoutEntry {
                //    binding: 3, // iChannelTex
                //    visibility: wgpu::ShaderStages::FRAGMENT,
                //    ty: wgpu::BindingType::Texture {
                //        multisampled: false,
                //        view_dimension: wgpu::TextureViewDimension::D2,
                //        sample_type: wgpu::TextureSampleType::Uint,
                //    },
                //    count: NonZeroU32::new(n_inputs),
                //},
            ],
            label: Some(&format!("EffectNode {} paint bind group layout", name)),
        });

        let render_pipeline_layout =
            ctx.device().create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Render Pipeline Layout"),
                bind_group_layouts: &[&update_bind_group_layout, &paint_bind_group_layout],
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
        let update_uniform_buffer = ctx.device().create_buffer(
            &wgpu::BufferDescriptor {
                label: Some(&format!("EffectNode {} update uniform buffer", name)),
                size: std::mem::size_of::<UpdateUniforms>() as u64,
                usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            }
        );

        // The paint uniform buffer for this effect
        let paint_uniform_buffer = ctx.device().create_buffer(
            &wgpu::BufferDescriptor {
                label: Some(&format!("EffectNode {} paint uniform buffer", name)),
                size: std::mem::size_of::<PaintUniforms>() as u64,
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

        // The update bind group is actually static, since we will just issue updates the uniform buffer
        let update_bind_group = ctx.device().create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &update_bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: update_uniform_buffer.as_entire_binding(),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&sampler),
                },
            ],
            label: Some(&format!("EffectNode {} update bind group", name)),
        });

        EffectNodeStateReady {
            name: name.to_string(),
            n_inputs,
            intensity_integral: 0.,
            render_pipeline,
            update_bind_group,
            paint_bind_group_layout,
            update_uniform_buffer,
            paint_uniform_buffer,
            paint_states: HashMap::new(),
        }
    }

    fn new_paint_state(self_ready: &EffectNodeStateReady, ctx: &Context, render_target_state: &RenderTargetState) -> EffectNodePaintState {
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
            input_textures: Vec::new(),
            output_texture: ArcTextureViewSampler::new(texture, view, sampler),
        }
    }

    fn update_paint_states(self_ready: &mut EffectNodeStateReady, ctx: &Context) {
        // See if we need to add or remove any paint states
        // (based on the context's render targets)

        // TODO add code to remove paint states, currently this only adds them

        for (check_render_target_id, render_target_state) in ctx.render_target_states().iter() {
            if !self_ready.paint_states.contains_key(check_render_target_id) {
                self_ready.paint_states.insert(*check_render_target_id, Self::new_paint_state(self_ready, ctx, render_target_state));
            }
        }
    }

    pub fn new(ctx: &Context, props: &EffectNodeProps) -> Self {
        // TODO kick of shader compilation in the background instead of blocking
        let mut new_obj_ready = Self::setup_render_pipeline(ctx, &props.name);
        Self::update_paint_states(&mut new_obj_ready, ctx);
        Self::Ready(new_obj_ready)
    }

    pub fn update(&mut self, ctx: &Context, props: &mut EffectNodeProps) {
        match self {
            EffectNodeState::Ready(self_ready) => {
                if props.name != self_ready.name {
                    *self = EffectNodeState::Error("EffectNode name changed after construction".to_string());
                    return;
                }

                Self::update_paint_states(self_ready, ctx);
                self_ready.intensity_integral = (self_ready.intensity_integral + props.intensity * ctx.dt()) % INTENSITY_INTEGRAL_PERIOD;

                let uniforms = UpdateUniforms {
                    audio: [0., 0., 0., 0.],
                    time: ctx.time(),
                    frequency: props.frequency,
                    intensity: props.intensity,
                    intensity_integral: self_ready.intensity_integral,
                    ..Default::default()
                };
                ctx.queue().write_buffer(&self_ready.update_uniform_buffer, 0, bytemuck::cast_slice(&[uniforms]));
            },
            _ => {}
        }
    }

    pub fn paint(&mut self, ctx: &Context, render_target_id: RenderTargetId) -> (Vec<wgpu::CommandBuffer>, ArcTextureViewSampler) {

        match self {
            EffectNodeState::Ready(self_ready) => {
                let render_target_state = ctx.render_target_state(render_target_id).expect("Call to paint() with a render target ID unknown to the context");
                let paint_state = self_ready.paint_states.get(&render_target_id).expect("Call to paint() with a render target ID unknown to the node (did you call update() first?)");

                // Populate the paint uniforms
                {
                    let width = render_target_state.width();
                    let height = render_target_state.height();
                    let uniforms = PaintUniforms {
                        resolution: [width as f32, height as f32],
                        dt: render_target_state.dt(),
                        ..Default::default()
                    };
                    ctx.queue().write_buffer(&self_ready.paint_uniform_buffer, 0, bytemuck::cast_slice(&[uniforms]));
                }

                // Populate the paint bind group

/* XXX allow inputs
                // Make an array of input textures
                // TODO repeatedly creating all these views seems bad,
                // but TextureViewArray takes in &[TextureView], not &[&TextureView] so it's hard.
                let input_binding: Vec<wgpu::TextureView> = (0..ready_state.n_inputs).map(|i| {
                    match inputs.get(i as usize) {
                        Some(opt_tex) => match opt_tex {
                            Some(tex) => tex.texture.create_view(&Default::default()),
                            None => context.blank_texture().texture.create_view(&Default::default()),
                        },
                        None => context.blank_texture().texture.create_view(&Default::default()),
                    }
                }).collect();
*/

                let paint_bind_group = ctx.device().create_bind_group(&wgpu::BindGroupDescriptor {
                    layout: &self_ready.paint_bind_group_layout,
                    entries: &[
                        wgpu::BindGroupEntry {
                            binding: 0, // PaintUniforms
                            resource: self_ready.paint_uniform_buffer.as_entire_binding(),
                        },
                        //wgpu::BindGroupEntry {
                        //    binding: 1, // iInputsTex
                        //    resource: wgpu::BindingResource::TextureViewArray(input_binding.as_slice())
                        //},
                        //wgpu::BindGroupEntry {
                        //    binding: 2, // iNoiseTex
                        //    resource: wgpu::BindingResource::TextureView(&context.noise_texture().view)
                        //},
                        //wgpu::BindGroupEntry {
                        //    binding: 3, // iChannelTex
                        //    resource: wgpu::BindingResource::TextureViewArray()
                        //},
                    ],
                    label: Some("EffectNode paint bind group"),
                });

                let mut encoder = ctx.device().create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("EffectNode encoder"),
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
                    render_pass.set_bind_group(0, &self_ready.update_bind_group, &[]); 
                    render_pass.set_bind_group(1, &paint_bind_group, &[]); 
                    render_pass.draw(0..4, 0..1);
                }

                (vec![encoder.finish()], paint_state.output_texture.clone())
            },
            _ => (vec![], ctx.blank_texture().clone()),
        }
        //return ctx.render_target_state(render_target_id).unwrap().noise_texture().clone(); // XXX actually paint something
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
