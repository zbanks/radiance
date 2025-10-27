use eframe::egui::Vec2;
use radiance::ArcTextureViewSampler;
use std::iter;

pub struct BeatWidget {
    // Constructor arguments:
    pixels_per_point: f32,

    // Internal state:
    width: u32,
    height: u32,
    texture: ArcTextureViewSampler,
    _shader_module: wgpu::ShaderModule,
    uniform_buffer: wgpu::Buffer,
    bind_group: wgpu::BindGroup,
    render_pipeline: wgpu::RenderPipeline,
}

// The uniform buffer associated with the ball
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Uniforms {
    resolution: [f32; 2], // in pixels
    size: [f32; 2],       // in points
    beat: f32,
    _padding: [u8; 4],
}

impl BeatWidget {
    fn make_texture(device: &wgpu::Device, width: u32, height: u32) -> ArcTextureViewSampler {
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
            view_formats: &[wgpu::TextureFormat::Rgba8Unorm],
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

    pub fn new(device: &wgpu::Device, pixels_per_point: f32) -> Self {
        let width = 1;
        let height = 1;
        let texture = Self::make_texture(&device, width, height);

        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(&"beat widget shader module"),
            source: wgpu::ShaderSource::Wgsl(include_str!("beat_widget.wgsl").into()),
        });

        // The uniform buffer for this widget
        let uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some(&"beat widget uniform buffer"),
            size: std::mem::size_of::<Uniforms>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some(&"beat widget bind group layout"),
            entries: &[wgpu::BindGroupLayoutEntry {
                binding: 0, // Uniforms
                visibility: wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Uniform,
                    has_dynamic_offset: false,
                    min_binding_size: None,
                },
                count: None,
            }],
        });

        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: uniform_buffer.as_entire_binding(),
            }],
            label: Some("beat bind group"),
        });

        let render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Beat widget render pipeline layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            });

        let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Beat widget render pipeline"),
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
                    blend: Some(wgpu::BlendState::REPLACE),
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

        Self {
            pixels_per_point,

            width,
            height,
            texture,
            _shader_module: shader_module,
            bind_group,
            uniform_buffer,
            render_pipeline,
        }
    }

    pub fn paint(
        &mut self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        size: Vec2,
        beat: f32,
    ) -> ArcTextureViewSampler {
        // Possibly remake the texture if the size has changed
        let width = (size.x * self.pixels_per_point) as u32;
        let height = (size.y * self.pixels_per_point) as u32;
        if width != self.width || height != self.height {
            self.width = width;
            self.height = height;
            self.texture = Self::make_texture(&device, width, height);
        }

        // Populate the uniforms
        let uniforms = Uniforms {
            resolution: [width as f32, height as f32],
            size: [size.x as f32, size.y as f32],
            beat,
            ..Default::default()
        };

        queue.write_buffer(&self.uniform_buffer, 0, bytemuck::cast_slice(&[uniforms]));

        let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
            label: Some("Beat widget encoder"),
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
                        store: wgpu::StoreOp::Store,
                    },
                    depth_slice: None,
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });

            render_pass.set_pipeline(&self.render_pipeline);
            render_pass.set_bind_group(0, &self.bind_group, &[]);
            render_pass.draw(0..4, 0..1);
        }

        // Submit the commands.
        queue.submit(iter::once(encoder.finish()));

        self.texture.clone()
    }
}
