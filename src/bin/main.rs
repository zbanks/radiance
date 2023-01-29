extern crate nalgebra as na;

use egui_winit::winit;
use egui_winit::winit::{
    event::*,
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};
use std::sync::Arc;
use std::iter;
use serde_json::json;
use egui_wgpu::renderer::{Renderer, ScreenDescriptor};
use std::collections::HashMap;

use radiance::{Context, RenderTargetList, RenderTargetId, Props, NodeId, Mir, NodeProps, EffectNodeProps, InsertionPoint};

mod ui;
use ui::{mosaic};

const BACKGROUND_COLOR: egui::Color32 = egui::Color32::from_rgb(51, 51, 51);

pub fn resize(new_size: winit::dpi::PhysicalSize<u32>, config: &mut wgpu::SurfaceConfiguration, device: &wgpu::Device, surface: &mut wgpu::Surface, screen_descriptor: Option<&mut ScreenDescriptor>) {
    if new_size.width > 0 && new_size.height > 0 {
        config.width = new_size.width;
        config.height = new_size.height;
        surface.configure(device, config);
        if let Some(screen_descriptor) = screen_descriptor {
            screen_descriptor.size_in_pixels = [config.width, config.height]
        }
    }
}

pub async fn run() {
    env_logger::init();
    let event_loop = EventLoop::new();
    let window = WindowBuilder::new().build(&event_loop).unwrap();
    let output_window = WindowBuilder::new().build(&event_loop).unwrap();

    let size = window.inner_size();
    let output_size = output_window.inner_size();

    // The instance is a handle to our GPU
    // Backends::all => Vulkan + Metal + DX12 + Browser WebGPU
    let instance = wgpu::Instance::new(wgpu::Backends::all());
    let mut surface = unsafe { instance.create_surface(&window) };
    let mut output_surface = unsafe { instance.create_surface(&output_window) };
    let adapter = instance.request_adapter(
        &wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::default(),
            compatible_surface: Some(&surface),
            force_fallback_adapter: false,
        },
    ).await.unwrap();

    let (device, queue) = adapter.request_device(
        &wgpu::DeviceDescriptor {
            features: wgpu::Features::TEXTURE_BINDING_ARRAY,
            // WebGL doesn't support all of wgpu's features, so if
            // we're building for the web we'll have to disable some.
            limits: if cfg!(target_arch = "wasm32") {
                wgpu::Limits::downlevel_webgl2_defaults()
            } else {
                wgpu::Limits::default()
            },
            label: None,
        },
        None, // Trace path
    ).await.unwrap();

    let device = Arc::new(device);
    let queue = Arc::new(queue);

    let mut config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format: surface.get_supported_formats(&adapter)[0],
        width: size.width,
        height: size.height,
        present_mode: wgpu::PresentMode::Fifo,
        alpha_mode: wgpu::CompositeAlphaMode::Auto,
    };
    let mut output_config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format: output_surface.get_supported_formats(&adapter)[0],
        width: output_size.width,
        height: output_size.height,
        present_mode: wgpu::PresentMode::Fifo,
        alpha_mode: wgpu::CompositeAlphaMode::Auto,
    };

    // EGUI
    let pixels_per_point = window.scale_factor() as f32;

    let mut screen_descriptor = ScreenDescriptor {
        size_in_pixels: [0, 0],
        pixels_per_point: window.scale_factor() as f32,
    };

    resize(size, &mut config, &device, &mut surface, Some(&mut screen_descriptor));
    resize(output_size, &mut output_config, &device, &mut output_surface, None);

    // Make a egui context:
    let egui_ctx = egui::Context::default();

    // We use the egui_winit_platform crate as the platform.
    let mut platform = egui_winit::State::new(&event_loop);
    platform.set_pixels_per_point(pixels_per_point);

    // We use the egui_wgpu_backend crate as the render backend.
    let mut egui_renderer = Renderer::new(&device, config.format, None, 1);

    // Output window WGPU stuff:

    let output_shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
        label: Some("Output shader"),
        source: wgpu::ShaderSource::Wgsl(include_str!("output.wgsl").into()),
    });

    let output_bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
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

    let output_render_pipeline_layout = device.create_pipeline_layout(
        &wgpu::PipelineLayoutDescriptor {
            label: Some("Output Render Pipeline Layout"),
            bind_group_layouts: &[&output_bind_group_layout],
            push_constant_ranges: &[],
        }
    );

     let output_render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
        label: Some("Output Render Pipeline"),
        layout: Some(&output_render_pipeline_layout),
        vertex: wgpu::VertexState {
            module: &output_shader_module,
            entry_point: "vs_main",
            buffers: &[],
        },
        fragment: Some(wgpu::FragmentState {
            module: &output_shader_module,
            entry_point: "fs_main",
            targets: &[Some(wgpu::ColorTargetState {
                format: output_config.format,
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

    // RADIANCE, WOO

    // Make a Mir
    let mut mir = Mir::new();

    // Make context
    let mut ctx = Context::new(device.clone(), queue.clone());

    // Make a graph
    let node1_id: NodeId = serde_json::from_value(json!("node_TW+qCFNoz81wTMca9jRIBg")).unwrap();
    let node2_id: NodeId = serde_json::from_value(json!("node_IjPuN2HID3ydxcd4qOsCuQ")).unwrap();
    let node3_id: NodeId = serde_json::from_value(json!("node_mW00lTCmDH/03tGyNv3iCQ")).unwrap();
    let node4_id: NodeId = serde_json::from_value(json!("node_EdpVLI4KG5JEBRNSgKUzsw")).unwrap();
    let node5_id: NodeId = serde_json::from_value(json!("node_I6AAXBaZKvSUfArs2vBr4A")).unwrap();
    let screen_output_node_id: NodeId = serde_json::from_value(json!("node_KSvPLGkiJDT+3FvPLf9JYQ")).unwrap();
    let mut props: Props = serde_json::from_value(json!({
        "graph": {
            "nodes": [
                node1_id,
                node2_id,
                node3_id,
                node4_id,
                node5_id,
                screen_output_node_id,
            ],
            "edges": [
                {
                    "from": node1_id,
                    "to": node2_id,
                    "input": 0,
                },
                {
                    "from": node2_id,
                    "to": node5_id,
                    "input": 1,
                },
                {
                    "from": node3_id,
                    "to": node4_id,
                    "input": 0,
                },
                {
                    "from": node4_id,
                    "to": node5_id,
                    "input": 0,
                },
                {
                    "from": node5_id,
                    "to": screen_output_node_id,
                    "input": 0,
                }
            ],
        },
        "node_props": {
            node1_id.to_string(): {
                "type": "EffectNode",
                "name": "purple",
                "input_count": 1,
                "intensity": 1.0,
            },
            node2_id.to_string(): {
                "type": "EffectNode",
                "name": "droste",
                "input_count": 1,
                "intensity": 1.0,
            },
            node3_id.to_string(): {
                "type": "EffectNode",
                "name": "wwave",
                "input_count": 1,
                "intensity": 0.6,
                "frequency": 0.25,
            },
            node4_id.to_string(): {
                "type": "EffectNode",
                "name": "zoomin",
                "input_count": 1,
                "intensity": 0.3,
                "frequency": 1.0
            },
            node5_id.to_string(): {
                "type": "EffectNode",
                "name": "uvmap",
                "input_count": 2,
                "intensity": 0.2,
                "frequency": 0.0
            },
            screen_output_node_id.to_string(): {
                "type": "ScreenOutputNode",
            }
        },
        "time": 0.,
        "dt": 0.03,
    })).unwrap();

    println!("Props: {}", serde_json::to_string(&props).unwrap());

    // Make a render targets
    let preview_render_target_id: RenderTargetId = serde_json::from_value(json!("rt_LVrjzxhXrGU7SqFo+85zkw")).unwrap();
    let output_render_target_id: RenderTargetId = serde_json::from_value(json!("rt_vvNth5LO1ZAUNLlJPiddNw")).unwrap();
    let render_target_list: RenderTargetList = serde_json::from_value(json!({
        preview_render_target_id.to_string(): {
            "width": 256,
            "height": 256,
            "dt": 1. / 60.
        },
        output_render_target_id.to_string(): {
            "width": 1920,
            "height": 1080,
            "dt": 1. / 60.
        }
    })).unwrap();

    println!("Render target list: {}", serde_json::to_string(&render_target_list).unwrap());

    // UI state
    let mut node_add_textedit = String::new();
    let mut left_panel_expanded = false;
    let mut node_add_wants_focus = false;
    let mut insertion_point: InsertionPoint = Default::default();

    event_loop.run(move |event, _, control_flow| {
        match event {
            Event::RedrawRequested(window_id) if window_id == window.id() => {
                // Update
                let music_info = mir.poll();
                props.time = music_info.time;
                props.dt = music_info.tempo * (1. / 60.);
                ctx.update(&mut props, &render_target_list);

                // Paint
                let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Encoder"),
                });

                let results = ctx.paint(&mut encoder, preview_render_target_id);

                let preview_images: HashMap<NodeId, egui::TextureId> = props.graph.nodes.iter().map(|&node_id| {
                    let tex_id = egui_renderer.register_native_texture(&device, &results.get(&node_id).unwrap().view, wgpu::FilterMode::Linear);
                    (node_id, tex_id)
                }).collect();

                // EGUI update
                let raw_input = platform.take_egui_input(&window);
                let full_output = egui_ctx.run(raw_input, |egui_ctx| {

                    let left_panel_response = egui::SidePanel::left("left").show_animated(egui_ctx, left_panel_expanded, |ui| {
                        ui.text_edit_singleline(&mut node_add_textedit)
                    });

                    egui::CentralPanel::default().show(egui_ctx, |ui| {
                        let mosaic_response = ui.add(mosaic("mosaic", &mut props, ctx.node_states(), &preview_images, &mut insertion_point));

                        if !left_panel_expanded && ui.input().key_pressed(egui::Key::A) {
                            left_panel_expanded = true;
                            node_add_wants_focus = true;
                        }

                        if let Some(egui::InnerResponse { inner: node_add_response, response: _}) = left_panel_response {
                            // TODO all this side-panel handling is wonky. It is done, in part, to avoid mutating the props before it's drawn.
                            // This needs to be factored out into a real "library" component.
                            if node_add_wants_focus {
                                node_add_response.request_focus();
                                node_add_wants_focus = false;
                            }
                            if node_add_response.lost_focus() {
                                if egui_ctx.input().key_pressed(egui::Key::Enter) {
                                    let new_node_id = NodeId::gen();
                                    let new_node_props = NodeProps::EffectNode(EffectNodeProps {
                                        name: node_add_textedit.clone(),
                                        ..Default::default()
                                    });
                                    props.node_props.insert(new_node_id, new_node_props);
                                    props.graph.insert_node(new_node_id, &insertion_point);
                                    // TODO: select and focus the new node
                                    // (consider making selection & focus part of the explicit state of mosaic, not memory)
                                }
                                node_add_textedit.clear();
                                left_panel_expanded = false;
                                mosaic_response.request_focus();
                            }
                        }
                    });
                });

                platform.handle_platform_output(&window, &egui_ctx, full_output.platform_output);
                let clipped_primitives = egui_ctx.tessellate(full_output.shapes); // create triangles to paint

                // EGUI paint
                let output = surface.get_current_texture().unwrap();
                let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

                // Upload all resources for the GPU.
                let tdelta: egui::TexturesDelta = full_output.textures_delta;
                for (texture_id, image_delta) in tdelta.set.iter() {
                    egui_renderer.update_texture(&device, &queue, *texture_id, image_delta);
                }
                egui_renderer.update_buffers(&device, &queue, &mut encoder, &clipped_primitives, &screen_descriptor);

                // Record UI render pass.
                {
                    let mut egui_render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: Some("EGUI Render Pass"),
                        color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                            view: &view,
                            resolve_target: None,
                            ops: wgpu::Operations {
                                load: wgpu::LoadOp::Clear(wgpu::Color {
                                    r: BACKGROUND_COLOR.r() as f64 / 255.,
                                    g: BACKGROUND_COLOR.g() as f64 / 255.,
                                    b: BACKGROUND_COLOR.b() as f64 / 255.,
                                    a: BACKGROUND_COLOR.a() as f64 / 255.,
                                }),
                                store: true,
                            },
                        })],
                        depth_stencil_attachment: None,
                    });

                    egui_renderer
                        .render(
                            &mut egui_render_pass,
                            &clipped_primitives,
                            &screen_descriptor,
                        );
                }

                // Submit the commands.
                queue.submit(iter::once(encoder.finish()));

                // Draw
                output.present();

                for texture_id in tdelta.free.iter() {
                    egui_renderer.free_texture(texture_id);
                }
            }
            Event::RedrawRequested(window_id) if window_id == output_window.id() => {
                // Paint
                let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Output Encoder"),
                });

                let results = ctx.paint(&mut encoder, output_render_target_id);

                if let Some(texture) = results.get(&screen_output_node_id) {
                    let output_bind_group = device.create_bind_group(
                        &wgpu::BindGroupDescriptor {
                            layout: &output_bind_group_layout,
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
                    let output = output_surface.get_current_texture().unwrap();
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

                        render_pass.set_pipeline(&output_render_pipeline);
                        render_pass.set_bind_group(0, &output_bind_group, &[]);
                        render_pass.draw(0..4, 0..1);

                        //egui_renderer
                        //    .render(
                        //        &mut egui_render_pass,
                        //        &clipped_primitives,
                        //        &screen_descriptor,
                        //    );
                    }

                    // Submit the commands.
                    queue.submit(iter::once(encoder.finish()));

                    // Draw
                    output.present();
                }
            }
            Event::MainEventsCleared => {
                // RedrawRequested will only trigger once, unless we manually
                // request it.
                window.request_redraw();
                output_window.request_redraw();
            }
            Event::WindowEvent {
                ref event,
                window_id,
            } if window_id == window.id() => if !false { // XXX
                // Pass the winit events to the EGUI platform integration.
                if platform.on_event(&egui_ctx, event).consumed {
                    return; // EGUI wants exclusive use of this event
                }
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
                        *control_flow = ControlFlow::Exit
                    },
                    WindowEvent::Resized(physical_size) => {
                        let size = *physical_size;
                        resize(size, &mut config, &device, &mut surface, Some(&mut screen_descriptor));
                    }
                    WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                        let size = **new_inner_size;
                        resize(size, &mut config, &device, &mut surface, Some(&mut screen_descriptor));
                    }
                    _ => {}
                }
            }
            Event::WindowEvent {
                ref event,
                window_id,
            } if window_id == output_window.id() => {
                match event {
                    WindowEvent::Resized(physical_size) => {
                        let output_size = *physical_size;
                        resize(output_size, &mut output_config, &device, &mut output_surface, None);
                    }
                    WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                        let output_size = **new_inner_size;
                        resize(output_size, &mut output_config, &device, &mut output_surface, None);
                    }
                    _ => {}
                }
            }
            _ => {}
        }
    });
}

pub fn main() {
    pollster::block_on(run());
}
