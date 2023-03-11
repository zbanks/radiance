use egui::{TextureId, Vec2};
use egui_wgpu::renderer::Renderer;
use radiance::{ArcTextureViewSampler, AudioLevels};
use std::iter;
use std::sync::Arc;

pub struct Widgets {
    // Constructor arguments:
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    pixels_per_point: f32,

    // Internal state:
    waveform_width: u32,
    waveform_height: u32,
    waveform_texture: ArcTextureViewSampler,
    waveform_shader_module: wgpu::ShaderModule,
    waveform_uniform_buffer: wgpu::Buffer,
    waveform_bind_group: wgpu::BindGroup,
    waveform_render_pipeline: wgpu::RenderPipeline,
}

// The uniform buffer associated with the waveform
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct WaveformUniforms {
    resolution: [f32; 2],
    _padding: [u8; 8],
}

impl Widgets {
    fn make_texture(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
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

    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>, pixels_per_point: f32) -> Self {
        let waveform_width = 1;
        let waveform_height = 1;
        let waveform_texture = Self::make_texture(&device, &queue, waveform_width, waveform_height);

        let waveform_shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(&"Waveform widget shader module"),
            source: wgpu::ShaderSource::Wgsl(include_str!("waveform_widget.wgsl").into()),
        });

        // The update uniform buffer for this effect
        let waveform_uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some(&"waveform widget uniform buffer"),
            size: std::mem::size_of::<WaveformUniforms>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let waveform_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some(&"waveform widget bind group layout"),
                entries: &[
                    wgpu::BindGroupLayoutEntry {
                        binding: 0, // WaveformUniforms
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Buffer {
                            ty: wgpu::BufferBindingType::Uniform,
                            has_dynamic_offset: false,
                            min_binding_size: None,
                        },
                        count: None,
                    },
                    /*wgpu::BindGroupLayoutEntry {
                        binding: 1, // iSampler
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                        count: None,
                    },
                    wgpu::BindGroupLayoutEntry {
                        binding: 2, // iWaveformTex
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Texture {
                            multisampled: false,
                            view_dimension: wgpu::TextureViewDimension::D2,
                            sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        },
                        count: None,
                    },*/
                ],
            });

        let waveform_bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &waveform_bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: waveform_uniform_buffer.as_entire_binding(),
            }],
            label: Some("output bind group"),
        });

        let waveform_render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Waveform widget render pipeline layout"),
                bind_group_layouts: &[&waveform_bind_group_layout],
                push_constant_ranges: &[],
            });

        let waveform_render_pipeline =
            device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("Waveform widget render pipeline"),
                layout: Some(&waveform_render_pipeline_layout),
                vertex: wgpu::VertexState {
                    module: &waveform_shader_module,
                    entry_point: "vs_main",
                    buffers: &[],
                },
                fragment: Some(wgpu::FragmentState {
                    module: &waveform_shader_module,
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

            waveform_width,
            waveform_height,
            waveform_texture,
            waveform_shader_module,
            waveform_bind_group,
            waveform_uniform_buffer,
            waveform_render_pipeline,
        }
    }

    pub fn waveform(
        &mut self,
        renderer: &mut Renderer,
        size: Vec2,
        audio: AudioLevels,
    ) -> TextureId {
        // Possibly remake the texture if the size has changed
        let width = (size.x * self.pixels_per_point) as u32;
        let height = (size.y * self.pixels_per_point) as u32;
        if width != self.waveform_width || height != self.waveform_height {
            self.waveform_width = width;
            self.waveform_height = height;
            self.waveform_texture = Self::make_texture(&self.device, &self.queue, width, height);
        }

        // Populate the uniforms and waveform data
        let uniforms = WaveformUniforms {
            ..Default::default()
        };

        self.queue.write_buffer(
            &self.waveform_uniform_buffer,
            0,
            bytemuck::cast_slice(&[uniforms]),
        );

        let mut encoder = self
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Waveform widget encoder"),
            });
        // Record output render pass.
        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Output window render pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &self.waveform_texture.view,
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

            render_pass.set_pipeline(&self.waveform_render_pipeline);
            render_pass.set_bind_group(0, &self.waveform_bind_group, &[]);
            render_pass.draw(0..4, 0..1);
        }

        // Submit the commands.
        self.queue.submit(iter::once(encoder.finish()));

        renderer.register_native_texture(
            &self.device,
            &self.waveform_texture.view,
            wgpu::FilterMode::Linear,
        )
    }
}
