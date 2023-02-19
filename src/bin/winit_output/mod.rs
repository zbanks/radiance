/// This module handles radiance output through winit
/// (e.g. actually displaying ScreenOutputNode to a screen)

use egui_winit::winit;
use egui_winit::winit::{
    event::*,
    event_loop::EventLoopWindowTarget,
    window::{WindowBuilder, Fullscreen},
};
use std::sync::Arc;
use std::iter;
use std::collections::HashMap;
use serde_json::json;

#[derive(Debug)]
pub struct WinitOutput {
    instance: Arc<wgpu::Instance>,
    adapter: Arc<wgpu::Adapter>,
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    shader_module: wgpu::ShaderModule,
    bind_group_layout: wgpu::BindGroupLayout,
    render_pipeline_layout: wgpu::PipelineLayout,

    screen_outputs: HashMap<radiance::NodeId, Option<VisibleScreenOutput>>,
}

#[derive(Debug)]
struct VisibleScreenOutput {
    // Resources
    window: egui_winit::winit::window::Window,
    surface: wgpu::Surface,
    config: wgpu::SurfaceConfiguration,
    render_pipeline: wgpu::RenderPipeline,
    render_target_id: radiance::RenderTargetId,
    render_target: radiance::RenderTarget,

    // Internal
    initial_update: bool, // Initialized to false, set to true on first update.
    request_close: bool, // If set to True, delete this window on the next update
}

impl VisibleScreenOutput {
    fn resize(&mut self, device: &wgpu::Device, new_size: winit::dpi::PhysicalSize<u32>) {
        if new_size.width > 0 && new_size.height > 0 {
            self.config.width = new_size.width;
            self.config.height = new_size.height;
            self.surface.configure(device, &self.config);
        }
    }
}

impl WinitOutput {
    pub fn new(instance: Arc<wgpu::Instance>, adapter: Arc<wgpu::Adapter>, device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {

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

        WinitOutput {
            instance,
            adapter,
            device,
            queue,
            shader_module,
            bind_group_layout,
            render_pipeline_layout,
            screen_outputs: HashMap::<radiance::NodeId, Option<VisibleScreenOutput>>::new(),
        }
    }

    pub fn render_targets_iter(&self) -> impl Iterator<Item=(&radiance::RenderTargetId, &radiance::RenderTarget)> {
        self.screen_outputs.values().flatten().map(|screen_output| (&screen_output.render_target_id, &screen_output.render_target))
    }

    pub fn update<T>(&mut self, event_loop: &EventLoopWindowTarget<T>, props: &mut radiance::Props) {
        // Mark all nodes that we know about as having received their initial update.
        // Painting is gated on this being true,
        // because otherwise, we might try to paint a render target that the radiance context doesn't know about.
        // After the initial update, the radiance context is guaranteed to know about this screen output's render target.
        for screen_output in self.screen_outputs.values_mut().flatten() {
            screen_output.initial_update = true;
        }

        // Prune screen_outputs of any nodes that are no longer present in the given graph
        self.screen_outputs.retain(|id, _| props.node_props.get(id).map(|node_props| matches!(node_props, radiance::NodeProps::ScreenOutputNode(_))).unwrap_or(false));

        // Construct screen_outputs for any ScreenOutputNodes we didn't know about
        for (node_id, node_props) in props.node_props.iter() {
            match node_props {
                radiance::NodeProps::ScreenOutputNode(_) => {
                    if !self.screen_outputs.contains_key(node_id) {
                        self.screen_outputs.insert(
                            *node_id,
                            None,
                        );
                    }
                },
                _ => {},
            }
        }

        // See what screens are available for output
        let screen_names: Vec<String> = event_loop.available_monitors().filter_map(|mh| mh.name()).collect();

        // Update internal state of screen_outputs from props
        let node_ids: Vec<radiance::NodeId> = self.screen_outputs.keys().cloned().collect();
        for node_id in node_ids {
            let screen_output_props: &mut radiance::ScreenOutputNodeProps = props.node_props.get_mut(&node_id).unwrap().try_into().unwrap();

            // If this node's screen list is totally empty, default it to the first screen
            if screen_output_props.available_screens.is_empty() && screen_output_props.screen.is_empty() && !screen_names.is_empty() {
                screen_output_props.screen = screen_names.first().unwrap().clone();
            }

            // Populate each screen output node props with a list of screens available on the system
            screen_output_props.available_screens = screen_names.clone();
            if !screen_names.contains(&screen_output_props.screen) {
                // Hide any outputs that point to screens we don't know about
                screen_output_props.visible = false;
            }
            if self.screen_outputs.get(&node_id).unwrap().as_ref().map(|visible_screen_output| visible_screen_output.request_close).unwrap_or(false) {
                // Close was requested; set visible to false
                screen_output_props.visible = false;
            }

            if screen_output_props.visible {
                if self.screen_outputs.get(&node_id).unwrap().is_none() {
                    // Newly visible window
                    let visible_screen_output = self.new_screen_output(event_loop);
                    let mh = event_loop.available_monitors().find(|mh| mh.name().map(|n| &n == &screen_output_props.screen).unwrap_or(false));
                    visible_screen_output.window.set_fullscreen(Some(Fullscreen::Borderless(mh.clone())));
                    // Replace None with Some
                    self.screen_outputs.insert(node_id, Some(visible_screen_output));
                }
            } else {
                if self.screen_outputs.get(&node_id).unwrap().is_some() {
                    // Replace Some with None
                    self.screen_outputs.insert(node_id, None);
                }
            }
        }
    }

    fn new_screen_output<T>(&self, event_loop: &EventLoopWindowTarget<T>) -> VisibleScreenOutput {
        let window = WindowBuilder::new().build(&event_loop).unwrap();
        let size = window.inner_size();
        let surface = unsafe { self.instance.create_surface(&window) };

        let config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: surface.get_supported_formats(&self.adapter)[0],
            width: size.width,
            height: size.height,
            present_mode: wgpu::PresentMode::Fifo,
            alpha_mode: wgpu::CompositeAlphaMode::Auto,
        };
        surface.configure(&self.device, &config);

        let render_pipeline = self.device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Output Render Pipeline"),
            layout: Some(&self.render_pipeline_layout),
            vertex: wgpu::VertexState {
                module: &self.shader_module,
                entry_point: "vs_main",
                buffers: &[],
            },
            fragment: Some(wgpu::FragmentState {
                module: &self.shader_module,
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

        let render_target_id = radiance::RenderTargetId::gen();
        let render_target: radiance::RenderTarget = serde_json::from_value(json!({
            "width": 1920,
            "height": 1080,
            "dt": 1. / 60.
        })).unwrap();

        VisibleScreenOutput {
            window,
            surface,
            config,
            render_pipeline,
            render_target_id,
            render_target,
            initial_update: false,
            request_close: false,
        }
    }

    pub fn on_event<T>(&mut self, event: &Event<T>, ctx: &mut radiance::Context) -> bool {
        // Return true => event consumed
        // Return false => event continues to be processed

        for (node_id, screen_output) in self.screen_outputs
            .iter_mut()
            .filter_map(|(k, v)| Some((k, v.as_mut()?))) {
            match event {
                Event::RedrawRequested(window_id) if window_id == &screen_output.window.id() => {
                    if screen_output.initial_update {
                        // Paint
                        let mut encoder = self.device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                            label: Some("Output Encoder"),
                        });

                        let results = ctx.paint(&mut encoder, screen_output.render_target_id);

                        if let Some(texture) = results.get(&node_id) {
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
                            let output = screen_output.surface.get_current_texture().unwrap();
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

                                render_pass.set_pipeline(&screen_output.render_pipeline);
                                render_pass.set_bind_group(0, &output_bind_group, &[]);
                                render_pass.draw(0..4, 0..1);
                            }

                            // Submit the commands.
                            self.queue.submit(iter::once(encoder.finish()));

                            // Draw
                            output.present();
                        }
                    }
                    return true;
                }
                Event::WindowEvent {
                    ref event,
                    window_id,
                } if window_id == &screen_output.window.id() => {
                    match event {
                        WindowEvent::CloseRequested
                        | WindowEvent::KeyboardInput {
                            input:
                                KeyboardInput {
                                    state: ElementState::Pressed,
                                    virtual_keycode: Some(VirtualKeyCode::Escape),
                                    ..
                                },
                            ..
                        } => {
                            screen_output.request_close = true;
                        }
                        WindowEvent::Resized(physical_size) => {
                            let output_size = *physical_size;
                            screen_output.resize(&self.device, output_size);
                        }
                        WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                            let output_size = **new_inner_size;
                            screen_output.resize(&self.device, output_size);
                        }
                        _ => {}
                    }
                    return true;
                }
                Event::MainEventsCleared => {
                    screen_output.window.request_redraw();
                }
                _ => {}
            }
        }
        false
    }
}
