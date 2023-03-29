use egui::Vec2;
use radiance::{ArcTextureViewSampler, SPECTRUM_LENGTH};
use std::iter;
use std::sync::Arc;

pub struct SpectrumWidget {
    // Constructor arguments:
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    pixels_per_point: f32,

    // Internal state:
    width: u32,
    height: u32,
    texture: ArcTextureViewSampler,
    _shader_module: wgpu::ShaderModule,
    uniform_buffer: wgpu::Buffer,
    bind_group: wgpu::BindGroup,
    render_pipeline: wgpu::RenderPipeline,
    data_texture: ArcTextureViewSampler,
}

// The uniform buffer associated with the spectrum
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Uniforms {
    resolution: [f32; 2], // in pixels
    size: [f32; 2],       // in points
    _padding: [u8; 8],
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Sample {
    pub spectrum: u8,
    _padding: [u8; 3],
}

impl SpectrumWidget {
    fn make_texture(
        device: &wgpu::Device,
        width: u32,
        height: u32,
    ) -> ArcTextureViewSampler {
        let texture_size = wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        };

        let texture_desc = wgpu::TextureDescriptor {
            size: texture_size,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::COPY_SRC
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::TEXTURE_BINDING,
            label: None,
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
    }

    fn make_data_texture(
        device: &wgpu::Device,
        size: u32,
    ) -> ArcTextureViewSampler {
        let texture_size = wgpu::Extent3d {
            width: size,
            height: 1,
            depth_or_array_layers: 1,
        };

        let texture_desc = wgpu::TextureDescriptor {
            size: texture_size,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D1,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::COPY_DST | wgpu::TextureUsages::TEXTURE_BINDING,
            label: None,
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
    }

    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>, pixels_per_point: f32) -> Self {
        let width = 1;
        let height = 1;
        let texture = Self::make_texture(&device, width, height);
        let data_texture = Self::make_data_texture(&device, SPECTRUM_LENGTH as u32);

        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(&"spectrum widget shader module"),
            source: wgpu::ShaderSource::Wgsl(include_str!("spectrum_widget.wgsl").into()),
        });

        // The update uniform buffer for this effect
        let uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some(&"spectrum widget uniform buffer"),
            size: std::mem::size_of::<Uniforms>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some(&"spectrum widget bind group layout"),
                entries: &[
                    wgpu::BindGroupLayoutEntry {
                        binding: 0, // Uniforms
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
                        binding: 2, // iSpectrumTex
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Texture {
                            multisampled: false,
                            view_dimension: wgpu::TextureViewDimension::D1,
                            sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        },
                        count: None,
                    },
                ],
            });

        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: uniform_buffer.as_entire_binding(),
                },
                wgpu::BindGroupEntry {
                    binding: 1, // iSampler
                    resource: wgpu::BindingResource::Sampler(&data_texture.sampler),
                },
                wgpu::BindGroupEntry {
                    binding: 2, // iSpectrumTex
                    resource: wgpu::BindingResource::TextureView(&data_texture.view),
                },
            ],
            label: Some("spectrum bind group"),
        });

        let render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Spectrum widget render pipeline layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            });

        let render_pipeline =
            device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("Spectrum widget render pipeline"),
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
                        blend: Some(wgpu::BlendState::REPLACE),
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

        Self {
            device,
            queue,
            pixels_per_point,

            width,
            height,
            texture,
            _shader_module: shader_module,
            bind_group,
            uniform_buffer,
            render_pipeline,
            data_texture,
        }
    }

    pub fn paint(&mut self, size: Vec2, spectrum: &[f32; SPECTRUM_LENGTH]) -> ArcTextureViewSampler {
        // Possibly remake the texture if the size has changed
        let width = (size.x * self.pixels_per_point) as u32;
        let height = (size.y * self.pixels_per_point) as u32;
        if width != self.width || height != self.height {
            self.width = width;
            self.height = height;
            self.texture = Self::make_texture(&self.device, width, height);
        }

        let data_texture_size = wgpu::Extent3d {
            width: SPECTRUM_LENGTH as u32,
            height: 1,
            depth_or_array_layers: 1,
        };

        // Populate the uniforms and spectrum data
        let uniforms = Uniforms {
            resolution: [width as f32, height as f32],
            size: [size.x as f32, size.y as f32],
            ..Default::default()
        };

        fn u8norm(x: f32) -> u8 {
            (x * 255.).clamp(0., 255.) as u8
        }

        let buffer: Vec<Sample> = spectrum.iter().map(|&s| Sample {
            spectrum: u8norm(s),
            ..Default::default()
        }).collect();

        self.queue.write_buffer(
            &self.uniform_buffer,
            0,
            bytemuck::cast_slice(&[uniforms]),
        );

        self.queue.write_texture(
            wgpu::ImageCopyTexture {
                texture: &self.data_texture.texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            bytemuck::cast_slice(&buffer),
            wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: std::num::NonZeroU32::new(4 * SPECTRUM_LENGTH as u32),
                rows_per_image: std::num::NonZeroU32::new(1),
            },
            data_texture_size,
        );

        let mut encoder = self
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Spectrum widget encoder"),
            });
        // Record output render pass.
        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Output window render pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &self.texture.view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.,
                            g: 0.,
                            b: 0.,
                            a: 0.,
                        }),
                        store: true,
                    },
                })],
                depth_stencil_attachment: None,
            });

            render_pass.set_pipeline(&self.render_pipeline);
            render_pass.set_bind_group(0, &self.bind_group, &[]);
            render_pass.draw(0..4, 0..1);
        }

        // Submit the commands.
        self.queue.submit(iter::once(encoder.finish()));

        self.texture.clone()
    }
}
