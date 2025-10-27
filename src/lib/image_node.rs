use crate::context::{ArcTextureViewSampler, Context, Fit, RenderTargetState};
use crate::render_target::RenderTargetId;
use crate::CommonNodeProps;
use image::GenericImageView;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::string::String;

const SHADER_SOURCE: &str = include_str!("image_shader.wgsl");

/// Properties of an ImageNode.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct ImageNodeProps {
    pub name: String,
    pub fit: Option<Fit>,
}

impl From<&ImageNodeProps> for CommonNodeProps {
    fn from(_props: &ImageNodeProps) -> Self {
        CommonNodeProps {
            input_count: Some(1),
        }
    }
}

#[allow(clippy::large_enum_variant)]
pub enum ImageNodeState {
    Uninitialized,
    Ready(ImageNodeStateReady),
    Error_(String), // ambiguous_associated_items error triggered by derive_more::TryInto without the _
}

pub struct ImageNodeStateReady {
    // Cached props
    name: String,
    fit: Fit,

    // Derived from file
    image_width: u32,
    image_height: u32,

    // GPU resources
    image_texture: ArcTextureViewSampler,
    bind_group_layout: wgpu::BindGroupLayout,
    uniform_buffer: wgpu::Buffer,
    sampler: wgpu::Sampler,
    render_pipeline: wgpu::RenderPipeline,

    // Paint states
    paint_states: HashMap<RenderTargetId, ImageNodePaintState>,
}

struct ImageNodePaintState {
    output_texture: ArcTextureViewSampler,
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Uniforms {
    factor: [f32; 2],
    _padding: [u8; 4],
}

// This is a state machine, it's more natural to use `match` than `if let`
#[allow(clippy::single_match)]
impl ImageNodeState {
    fn setup_render_pipeline(
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        props: &ImageNodeProps,
    ) -> Result<ImageNodeStateReady, String> {
        let name = &props.name;

        // Image
        let image_name = format!("library/{name}");
        let image_data = ctx
            .fetch_content_bytes(&image_name)
            .map_err(|_| format!("Failed to read image file \"{image_name}\""))?;

        let image_obj = image::load_from_memory(&image_data).unwrap();
        let image_rgba = image_obj.to_rgba8();
        let (image_width, image_height) = image_obj.dimensions();
        let image_size = wgpu::Extent3d {
            width: image_width,
            height: image_height,
            depth_or_array_layers: 1,
        };

        let image_texture = {
            let texture_desc = wgpu::TextureDescriptor {
                size: image_size,
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8UnormSrgb,
                usage: wgpu::TextureUsages::COPY_DST | wgpu::TextureUsages::TEXTURE_BINDING,
                label: Some("image"),
                view_formats: &[wgpu::TextureFormat::Rgba8UnormSrgb],
            };
            let texture = device.create_texture(&texture_desc);
            let view = texture.create_view(&Default::default());
            let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Linear,
                min_filter: wgpu::FilterMode::Linear,
                mipmap_filter: wgpu::FilterMode::Linear,
                ..Default::default()
            });
            ArcTextureViewSampler::new(texture, view, sampler)
        };

        // Write the image
        queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture: &image_texture.texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &image_rgba,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(4 * image_size.width),
                rows_per_image: Some(image_size.height),
            },
            image_size,
        );

        device.push_error_scope(wgpu::ErrorFilter::Validation);
        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(&format!("ImageNode {}", name)),
            source: wgpu::ShaderSource::Wgsl(SHADER_SOURCE.into()),
        });

        let result = pollster::block_on(device.pop_error_scope());
        if let Some(error) = result {
            return Err(format!("ImageNode shader compilation error: {}\n", error));
        }

        // The uniforms bind group:
        // 0: Uniforms
        // 1: iSampler
        // 2: iInputTex
        // 3: iImageTex

        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0, // Uniforms
                    visibility: wgpu::ShaderStages::VERTEX,
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
                    binding: 2, // iInputTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 3, // iImageTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    },
                    count: None,
                },
            ],
            label: Some(&format!("ImageNode {} bind group layout", name)),
        });

        let render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Render Pipeline Layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            });

        // Create a render pipeline
        let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some(&format!("ImageNode {} render pipeline", name)),
            layout: Some(&render_pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader_module,
                entry_point: Some("vs_main"),
                buffers: &[],
                compilation_options: Default::default(),
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader_module,
                entry_point: Some("fs_main"),
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    blend: None,
                    write_mask: wgpu::ColorWrites::ALL,
                })],
                compilation_options: Default::default(),
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
            cache: None,
        });

        // The update uniform buffer for this effect
        let uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some(&format!("ImageNode {} uniform buffer", name)),
            size: std::mem::size_of::<Uniforms>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        // The sampler that will be used for texture access within the shaders
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });

        Ok(ImageNodeStateReady {
            name: name.clone(),
            fit: Fit::Shrink,
            image_width,
            image_height,
            image_texture,
            bind_group_layout,
            uniform_buffer,
            sampler,
            render_pipeline,
            paint_states: HashMap::new(),
        })
    }

    fn new_paint_state(
        _self_ready: &ImageNodeStateReady,
        _ctx: &Context,
        device: &wgpu::Device,
        _queue: &wgpu::Queue,
        render_target_state: &RenderTargetState,
    ) -> ImageNodePaintState {
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
                | wgpu::TextureUsages::TEXTURE_BINDING,
            label: None,
            view_formats: &[wgpu::TextureFormat::Rgba8Unorm],
        };

        let make_texture = || {
            let texture = device.create_texture(&texture_desc);
            let view = texture.create_view(&Default::default());
            let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Linear,
                min_filter: wgpu::FilterMode::Linear,
                mipmap_filter: wgpu::FilterMode::Linear,
                ..Default::default()
            });
            ArcTextureViewSampler::new(texture, view, sampler)
        };

        let output_texture = make_texture();

        ImageNodePaintState { output_texture }
    }

    fn update_paint_states(
        self_ready: &mut ImageNodeStateReady,
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
    ) {
        // See if we need to add or remove any paint states
        // (based on the context's render targets)

        self_ready
            .paint_states
            .retain(|id, _| ctx.render_target_states().contains_key(id));

        for (check_render_target_id, render_target_state) in ctx.render_target_states().iter() {
            if !self_ready.paint_states.contains_key(check_render_target_id) {
                self_ready.paint_states.insert(
                    *check_render_target_id,
                    Self::new_paint_state(self_ready, ctx, device, queue, render_target_state),
                );
            }
        }
    }

    pub fn new(
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        props: &ImageNodeProps,
    ) -> Self {
        // TODO kick of shader compilation in the background instead of blocking
        match Self::setup_render_pipeline(ctx, device, queue, props) {
            Ok(mut new_obj_ready) => {
                Self::update_paint_states(&mut new_obj_ready, ctx, device, queue);
                Self::Ready(new_obj_ready)
            }
            Err(msg) => {
                eprintln!("Unable to configure ImageNode: {}", msg);
                Self::Error_(msg)
            }
        }
    }

    pub fn update(
        &mut self,
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        props: &mut ImageNodeProps,
    ) {
        match self {
            ImageNodeState::Ready(self_ready) => {
                if props.name != self_ready.name {
                    *self = ImageNodeState::Error_(
                        "ImageNode name changed after construction".to_string(),
                    );
                    return;
                }
                match &props.fit {
                    Some(fit) => {
                        self_ready.fit = fit.clone();
                    }
                    _ => {}
                }

                // Report back to the caller what our props are
                self_ready.update_props(props);

                Self::update_paint_states(self_ready, ctx, device, queue);
            }
            _ => {}
        }
    }

    pub fn paint(
        &mut self,
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        encoder: &mut wgpu::CommandEncoder,
        render_target_id: RenderTargetId,
        inputs: &[Option<ArcTextureViewSampler>],
    ) -> ArcTextureViewSampler {
        match self {
            ImageNodeState::Ready(self_ready) => {
                let paint_state = self_ready.paint_states.get_mut(&render_target_id).expect("Call to paint() with a render target ID unknown to the node (did you call update() first?)");

                let render_target_state = ctx
                    .render_target_state(render_target_id)
                    .expect("Call to paint() with a render target ID unknown to the context");

                let media_size = (
                    self_ready.image_width as f32,
                    self_ready.image_height as f32,
                );
                let canvas_size = (
                    render_target_state.width() as f32,
                    render_target_state.height() as f32,
                );
                let (factor_fit_x, factor_fit_y) = self_ready.fit.factor(media_size, canvas_size);

                // Populate the uniforms
                {
                    let uniforms = Uniforms {
                        factor: [factor_fit_x, factor_fit_y],
                        ..Default::default()
                    };
                    queue.write_buffer(
                        &self_ready.uniform_buffer,
                        0,
                        bytemuck::cast_slice(&[uniforms]),
                    );
                }

                // Make an array of input textures
                let input_texture = if let Some(Some(tex)) = inputs.get(0) {
                    tex
                } else {
                    ctx.blank_texture()
                };

                let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
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
                            binding: 2, // iInputTex
                            resource: wgpu::BindingResource::TextureView(&input_texture.view),
                        },
                        wgpu::BindGroupEntry {
                            binding: 3, // iImageTex
                            resource: wgpu::BindingResource::TextureView(
                                &self_ready.image_texture.view,
                            ),
                        },
                    ],
                    label: Some("ImageNode bind group"),
                });

                {
                    let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: Some("ImageNode render pass"),
                        color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                            view: paint_state.output_texture.view.as_ref(),
                            resolve_target: None,
                            ops: wgpu::Operations {
                                load: wgpu::LoadOp::Load,
                                store: wgpu::StoreOp::Store,
                            },
                            depth_slice: None,
                        })],
                        depth_stencil_attachment: None,
                        timestamp_writes: None,
                        occlusion_query_set: None,
                    });

                    render_pass.set_pipeline(&self_ready.render_pipeline);
                    render_pass.set_bind_group(0, &bind_group, &[]);
                    render_pass.draw(0..4, 0..1);
                }

                paint_state.output_texture.clone()
            }

            _ => ctx.blank_texture().clone(),
        }
    }
}

impl ImageNodeStateReady {
    fn update_props(&self, props: &mut ImageNodeProps) {
        props.name.clone_from(&self.name);
        props.fit = Some(self.fit.clone());
    }
}
