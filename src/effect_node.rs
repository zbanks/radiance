use std::string::String;
use std::num::NonZeroU32;
use crate::context::{Context, ArcTextureViewSampler};
use crate::render_target::RenderTargetId;

pub struct EffectNodeProps {
    pub name: String,
    pub intensity: f32,
}

pub enum EffectNodeStatus {
}

pub enum EffectNodeState {
    Uninitialized,
    Ready(ReadyState),
    Error(String),
}

pub struct ReadyState {
    render_pipeline: wgpu::RenderPipeline,
    update_bind_group: wgpu::BindGroup,
    paint_bind_group_layout: wgpu::BindGroupLayout,
    update_uniform_buffer: wgpu::Buffer,
    paint_uniform_buffer: wgpu::Buffer,
    n_inputs: u32,
}

const EFFECT_HEADER: &str = include_str!("effect_header.wgsl");
const EFFECT_FOOTER: &str = include_str!("effect_footer.wgsl");

struct EffectNodePaintState {
//    width: u32,
//    height: u32,
//    texture: Arc<wgpu::Texture>,
}

// The uniform buffer associated with the effect (agnostic to render target)
#[repr(C)]
#[derive(Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
#[allow(non_snake_case)]
struct UpdateUniforms {
    iAudio: [f32; 4],
    iStep: f32,
    iTime: f32,
    iFrequency: f32,
    iIntensity: f32,
    iIntensityIntegral: f32,
}

// The uniform buffer associated with the effect (specific to render target)
#[repr(C)]
#[derive(Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
#[allow(non_snake_case)]
struct PaintUniforms {
    iResolution: [f32; 2],
    iFPS: f32,
}

impl EffectNodeState {
    fn setup_render_pipeline(ctx: &Context, name: &str) -> Self {
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
                wgpu::BindGroupLayoutEntry {
                    binding: 1, // iInputsTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Uint,
                    },
                    count: NonZeroU32::new(n_inputs),
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 2, // iNoiseTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Uint,
                    },
                    count: None,
                },
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
                    format: wgpu::TextureFormat::Rgba32Float,
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

        EffectNodeState::Ready(ReadyState {
            render_pipeline,
            update_bind_group,
            paint_bind_group_layout,
            update_uniform_buffer,
            paint_uniform_buffer,
            n_inputs,
        })
    }

    pub fn new(ctx: &Context, props: &EffectNodeProps) -> Self {
        // TODO kick of shader compilation in the background instead of blocking
        Self::setup_render_pipeline(ctx, &props.name)
    }

    pub fn update(&mut self, ctx: &Context, props: &EffectNodeProps, time: f32) {
    }

    pub fn paint(&mut self, ctx: &Context, render_target_id: RenderTargetId) -> ArcTextureViewSampler {
        return ctx.render_target_state(render_target_id).unwrap().noise_texture().clone(); // XXX actually paint something
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
