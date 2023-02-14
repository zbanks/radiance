/// This module handles radiance output through winit
/// (e.g. actually displaying ScreenOutputNode to a screen)

use egui_winit::winit;
use egui_winit::winit::{
    event::*,
    event_loop::EventLoop,
    window::WindowBuilder,
};
use std::sync::Arc;
use std::iter;

pub struct WinitOutput {
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    window: egui_winit::winit::window::Window,
    surface: wgpu::Surface,
    config: wgpu::SurfaceConfiguration,
    _shader_module: wgpu::ShaderModule,
    bind_group_layout: wgpu::BindGroupLayout,
    _render_pipeline_layout: wgpu::PipelineLayout,
    render_pipeline: wgpu::RenderPipeline,
}

impl WinitOutput {
    pub fn new<T>(event_loop: &EventLoop<T>, instance: &wgpu::Instance, adapter: &wgpu::Adapter, device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {
        let window = WindowBuilder::new().build(&event_loop).unwrap();
        let size = window.inner_size();
        let surface = unsafe { instance.create_surface(&window) };

        let config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: surface.get_supported_formats(&adapter)[0],
            width: size.width,
            height: size.height,
            present_mode: wgpu::PresentMode::Fifo,
            alpha_mode: wgpu::CompositeAlphaMode::Auto,
        };
        surface.configure(&device, &config);

        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Output shader"),
            source: wgpu::ShaderSource::Wgsl(include_str!("output.wgsl").into()),
        });

        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
            ],
            label: Some("output texture bind group layout"),
        });

        let render_pipeline_layout = device.create_pipeline_layout(
            &wgpu::PipelineLayoutDescriptor {
                label: Some("Output Render Pipeline Layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            }
        );

         let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Output Render Pipeline"),
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
                    format: config.format,
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

        WinitOutput {
            device,
            queue,
            window,
            surface,
            config,
            _shader_module: shader_module,
            bind_group_layout,
            _render_pipeline_layout: render_pipeline_layout,
            render_pipeline,
        }
    }

    pub fn resize(&mut self, new_size: winit::dpi::PhysicalSize<u32>) {
        if new_size.width > 0 && new_size.height > 0 {
            self.config.width = new_size.width;
            self.config.height = new_size.height;
            self.surface.configure(&self.device, &self.config);
        }
    }

    pub fn on_event<T>(&mut self, event: &Event<T>, ctx: &mut radiance::Context, render_target_id: radiance::RenderTargetId, screen_output_node_id: radiance::NodeId) -> bool {
        // Return true => event consumed
        // Return false => event continues to be processed
        match event {
            Event::RedrawRequested(window_id) if window_id == &self.window.id() => {
                // Paint
                let mut encoder = self.device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Output Encoder"),
                });

                let results = ctx.paint(&mut encoder, render_target_id);

                if let Some(texture) = results.get(&screen_output_node_id) {
                    let output_bind_group = self.device.create_bind_group(
                        &wgpu::BindGroupDescriptor {
                            layout: &self.bind_group_layout,
                            entries: &[
                                wgpu::BindGroupEntry {
                                    binding: 0,
                                    resource: wgpu::BindingResource::TextureView(&texture.view),
                                },
                                wgpu::BindGroupEntry {
                                    binding: 1,
                                    resource: wgpu::BindingResource::Sampler(&texture.sampler),
                                }
                            ],
                            label: Some("output bind group"),
                        }
                    );

                    // Record output render pass.
                    let output = self.surface.get_current_texture().unwrap();
                    let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

                    {
                        let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                            label: Some("Output window render pass"),
                            color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                                view: &view,
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
                        render_pass.set_bind_group(0, &output_bind_group, &[]);
                        render_pass.draw(0..4, 0..1);
                    }

                    // Submit the commands.
                    self.queue.submit(iter::once(encoder.finish()));

                    // Draw
                    output.present();
                }
                true
            }
            Event::MainEventsCleared => {
                self.window.request_redraw();
                false
            }
            Event::WindowEvent {
                ref event,
                window_id,
            } if window_id == &self.window.id() => {
                match event {
                    WindowEvent::Resized(physical_size) => {
                        let output_size = *physical_size;
                        self.resize(output_size);
                    }
                    WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                        let output_size = **new_inner_size;
                        self.resize(output_size);
                    }
                    _ => {}
                }
                true
            }
            _ => {false}
        }
    }
}
