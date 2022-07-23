extern crate nalgebra as na;

use na::{Vector2, Matrix4};
use radiance::ArcTextureViewSampler;
use std::sync::Arc;

// The uniform buffer associated with the node preview
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Uniforms {
    view: [f32; 16],
    pos_min: [f32; 2],
    pos_max: [f32; 2],
    _padding: [u8; 0],
}

//////////////////////////////////////////////////////////////////////////////

#[derive(Debug, Clone)]
struct InstanceDescriptor {
    texture: ArcTextureViewSampler,
    pos_min: Vector2<f32>,
    pos_max: Vector2<f32>,
}

struct InstanceResources {
    uniform_buffer: wgpu::Buffer,
}

pub struct RenderPassResources {
    bind_groups: Vec<wgpu::BindGroup>,
}

pub struct VideoNodePreviewRenderer {
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    pipeline: wgpu::RenderPipeline,
    bind_group_layout: wgpu::BindGroupLayout,

    view: Matrix4<f32>,
    instance_list: Vec<InstanceDescriptor>,
    instance_cache: Vec<InstanceResources>,
}

impl VideoNodePreviewRenderer {
    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Shader"),
            source: wgpu::ShaderSource::Wgsl(include_str!("video_node_preview.wgsl").into()),
        });

        let bind_group_layout = device.create_bind_group_layout(
            &wgpu::BindGroupLayoutDescriptor {
                entries: &[
                    wgpu::BindGroupLayoutEntry {
                        binding: 0,
                        visibility: wgpu::ShaderStages::VERTEX,
                        ty: wgpu::BindingType::Buffer {
                            ty: wgpu::BufferBindingType::Uniform,
                            has_dynamic_offset: false,
                            min_binding_size: None,
                        },
                        count: None,
                    },
                    wgpu::BindGroupLayoutEntry {
                        binding: 1,
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Texture {
                            multisampled: false,
                            view_dimension: wgpu::TextureViewDimension::D2,
                            sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        },
                        count: None,
                    },
                    wgpu::BindGroupLayoutEntry {
                        binding: 2,
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                        count: None,
                    },
                ],
                label: Some("screen bind group layout"),
            }
        );

        let pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Screen Render Pipeline Layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            });

        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Screen Render Pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: "fs_main",
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Bgra8UnormSrgb,
                    blend: Some(wgpu::BlendState::PREMULTIPLIED_ALPHA_BLENDING),
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
            pipeline,
            bind_group_layout,

            view: Matrix4::identity(),
            instance_cache: Default::default(),
            instance_list: Default::default(),
        }
    }

    fn new_instance(&mut self) -> InstanceResources {
        // The uniform buffer for the video node preview
        let uniform_buffer = self.device.create_buffer(
            &wgpu::BufferDescriptor {
                label: Some("video node preview uniform buffer"),
                size: std::mem::size_of::<Uniforms>() as u64,
                usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            }
        );

        InstanceResources {
            uniform_buffer,
        }
    }

    pub fn set_view(&mut self, view: &Matrix4<f32>) {
        self.view = *view;
    }

    pub fn push_instance(&mut self, texture: &ArcTextureViewSampler, pos_min: &Vector2<f32>, pos_max: &Vector2<f32>) {
        self.instance_list.push(
            InstanceDescriptor {
                texture: texture.clone(),
                pos_min: pos_min.clone(),
                pos_max: pos_max.clone(),
            },
        );
    }

    // Modeled off of https://github.com/gfx-rs/wgpu-rs/wiki/Encapsulating-Graphics-Work

    pub fn prepare(&mut self) -> RenderPassResources {

        if self.instance_list.len() > self.instance_cache.len() {
            // If we don't have enough cache entries, add them
            for _ in 0..self.instance_list.len() - self.instance_cache.len() {
                let new = self.new_instance();
                self.instance_cache.push(new);
            }
        } else if self.instance_cache.len() > self.instance_list.len() {
            // If we have too many cache entries, remove them
            for _ in 0..self.instance_cache.len() - self.instance_list.len() {
                self.instance_cache.pop();
            }
        }

        let mut bind_groups = Vec::<wgpu::BindGroup>::new();

        for i in 0..self.instance_list.len() {
            // Paint
            let descriptor = &mut self.instance_list[i];
            let resources = &mut self.instance_cache[i];

            let uniforms = Uniforms {
                view: [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.],
                pos_min: [0., 0.],
                pos_max: [0.5, 0.5],
                ..Default::default()
            };
            self.queue.write_buffer(&resources.uniform_buffer, 0, bytemuck::cast_slice(&[uniforms]));

            let bind_group = self.device.create_bind_group(
                &wgpu::BindGroupDescriptor {
                    layout: &self.bind_group_layout,
                    entries: &[
                        wgpu::BindGroupEntry {
                            binding: 0,
                            resource: resources.uniform_buffer.as_entire_binding(),
                        },
                        wgpu::BindGroupEntry {
                            binding: 1,
                            resource: wgpu::BindingResource::TextureView(&descriptor.texture.view),
                        },
                        wgpu::BindGroupEntry {
                            binding: 2,
                            resource: wgpu::BindingResource::Sampler(&descriptor.texture.sampler),
                        },
                    ],
                    label: Some("video node preview bind group"),
                }
            );
            bind_groups.push(bind_group);
        }

        // Clear the list
        self.instance_list.clear();
        RenderPassResources {
            bind_groups
        }
    }

    pub fn paint<'rpass>(&'rpass self, render_pass: &mut wgpu::RenderPass<'rpass>, render_pass_resources: &'rpass RenderPassResources) {
        for bind_group in render_pass_resources.bind_groups.iter() {
            render_pass.set_pipeline(&self.pipeline);
            render_pass.set_bind_group(0, bind_group, &[]);
            render_pass.draw(0..4, 0..1);
        }
    }
}
