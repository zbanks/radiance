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
use egui_wgpu::renderer::{RenderPass, ScreenDescriptor};

use radiance::{Context, RenderTargetList, RenderTargetId, Graph, NodeId, NodeState, Mir, NodeProps};

mod ui;
use ui::{Tile, EffectNodeTile};

const BACKGROUND_COLOR: egui::Color32 = egui::Color32::from_rgb(51, 51, 51);

pub fn resize(new_size: winit::dpi::PhysicalSize<u32>, config: &mut wgpu::SurfaceConfiguration, device: &wgpu::Device, surface: &mut wgpu::Surface, screen_descriptor: &mut ScreenDescriptor) {
    if new_size.width > 0 && new_size.height > 0 {
        config.width = new_size.width;
        config.height = new_size.height;
        surface.configure(device, config);
        screen_descriptor.size_in_pixels = [config.width, config.height]
    }
}

pub async fn run() {
    env_logger::init();
    let event_loop = EventLoop::new();
    let window = WindowBuilder::new().build(&event_loop).unwrap();

    let mut size = window.inner_size();

    // The instance is a handle to our GPU
    // Backends::all => Vulkan + Metal + DX12 + Browser WebGPU
    let instance = wgpu::Instance::new(wgpu::Backends::all());
    let mut surface = unsafe { instance.create_surface(&window) };
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
    };

    // EGUI
    let pixels_per_point = window.scale_factor() as f32;

    let mut screen_descriptor = ScreenDescriptor {
        size_in_pixels: [0, 0],
        pixels_per_point: window.scale_factor() as f32,
    };

    resize(size, &mut config, &device, &mut surface, &mut screen_descriptor);

    // Make a egui context:
    let egui_ctx = egui::Context::default();

    // We use the egui_winit_platform crate as the platform.
    let mut platform = egui_winit::State::new(&event_loop);
    platform.set_pixels_per_point(pixels_per_point);

    // We use the egui_wgpu_backend crate as the render backend.
    let mut egui_rpass = RenderPass::new(&device, config.format, 1);

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
    let mut graph: Graph = serde_json::from_value(json!({
        "nodes": [
            node1_id,
            node2_id,
            node3_id,
            node4_id,
            node5_id,
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
            }
        ],
        "node_props": {
            node1_id.to_string(): {
                "type": "EffectNode",
                "name": "purple.wgsl",
                "intensity": 1.0,
                "frequency": 1.0
            },
            node2_id.to_string(): {
                "type": "EffectNode",
                "name": "droste.wgsl",
                "intensity": 1.0,
                "frequency": 1.0
            },
            node3_id.to_string(): {
                "type": "EffectNode",
                "name": "wwave.wgsl",
                "intensity": 0.6,
                "frequency": 0.25,
            },
            node4_id.to_string(): {
                "type": "EffectNode",
                "name": "zoomin.wgsl",
                "intensity": 0.3,
                "frequency": 1.0
            },
            node5_id.to_string(): {
                "type": "EffectNode",
                "name": "uvmap.wgsl",
                "intensity": 0.2,
                "frequency": 1.0
            }
        },
        "global_props": {
            "time": 0.,
            "dt": 0.03,
        }
    })).unwrap();

    println!("Graph: {}", serde_json::to_string(&graph).unwrap());

    // Make a render target
    let preview_render_target_id: RenderTargetId = serde_json::from_value(json!("rt_LVrjzxhXrGU7SqFo+85zkw")).unwrap();
    let render_target_list: RenderTargetList = serde_json::from_value(json!({
        preview_render_target_id.to_string(): {
            "width": 256,
            "height": 256,
            "dt": 1. / 60.
        }
    })).unwrap();

    println!("Render target list: {}", serde_json::to_string(&render_target_list).unwrap());

    event_loop.run(move |event, _, control_flow| {
        match event {
            Event::RedrawRequested(window_id) if window_id == window.id() => {
                // Update
                let music_info = mir.poll();
                graph.global_props_mut().time = music_info.time;
                graph.global_props_mut().dt = music_info.tempo * (1. / 60.);
                ctx.update(&mut graph, &render_target_list);

                // Paint
                let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Encoder"),
                });

                let results = ctx.paint(&mut encoder, preview_render_target_id);

                // Get node states
                let node_state_1 = if let NodeState::EffectNode(s) = ctx.node_state(node1_id).unwrap() {s} else {panic!("Not EffectNode!")};
                let node_state_2 = if let NodeState::EffectNode(s) = ctx.node_state(node2_id).unwrap() {s} else {panic!("Not EffectNode!")};
                let node_state_3 = if let NodeState::EffectNode(s) = ctx.node_state(node3_id).unwrap() {s} else {panic!("Not EffectNode!")};
                let node_state_4 = if let NodeState::EffectNode(s) = ctx.node_state(node4_id).unwrap() {s} else {panic!("Not EffectNode!")};
                let node_state_5 = if let NodeState::EffectNode(s) = ctx.node_state(node4_id).unwrap() {s} else {panic!("Not EffectNode!")};

                // Get node outputs
                let mut preview = |node_id| {
                    egui_rpass.register_native_texture(&device, &results.get(node_id).unwrap().view, wgpu::FilterMode::Linear)
                };

                let preview_texture_1 = preview(&node1_id);
                let preview_texture_2 = preview(&node2_id);
                let preview_texture_3 = preview(&node3_id);
                let preview_texture_4 = preview(&node4_id);
                let preview_texture_5 = preview(&node5_id);

                // EGUI update
                let raw_input = platform.take_egui_input(&window);
                let full_output = egui_ctx.run(raw_input, |egui_ctx| {
                    egui::CentralPanel::default().show(&egui_ctx, |ui| {
                        {
                            let node_props_1 = if let NodeProps::EffectNode(s) = graph.node_props_mut(&node1_id).unwrap() {s} else {panic!("Not EffectNode!")};
                            ui.add(EffectNodeTile::new(
                                Tile::new(
                                    egui::Rect::from_min_size(egui::pos2(100., 320.), egui::vec2(130., 200.)),
                                    &[100.],
                                    &[100.]
                                ),
                                &node_props_1.name,
                                preview_texture_1,
                                &mut node_props_1.intensity,
                            ));
                        }
                        {
                            let node_props_2 = if let NodeProps::EffectNode(s) = graph.node_props_mut(&node2_id).unwrap() {s} else {panic!("Not EffectNode!")};
                            ui.add(EffectNodeTile::new(
                                Tile::new(
                                    egui::Rect::from_min_size(egui::pos2(230., 320.), egui::vec2(130., 200.)),
                                    &[100.],
                                    &[100.]
                                ),
                                &node_props_2.name,
                                preview_texture_2,
                                &mut node_props_2.intensity,
                            ));
                        }
                        {
                            let node_props_3 = if let NodeProps::EffectNode(s) = graph.node_props_mut(&node3_id).unwrap() {s} else {panic!("Not EffectNode!")};
                            ui.add(EffectNodeTile::new(
                                Tile::new(
                                    egui::Rect::from_min_size(egui::pos2(100., 100.), egui::vec2(130., 200.)),
                                    &[100.],
                                    &[100.]
                                ),
                                &node_props_3.name,
                                preview_texture_3,
                                &mut node_props_3.intensity,
                            ));
                        }
                        {
                            let node_props_4 = if let NodeProps::EffectNode(s) = graph.node_props_mut(&node4_id).unwrap() {s} else {panic!("Not EffectNode!")};
                            ui.add(EffectNodeTile::new(
                                Tile::new(
                                    egui::Rect::from_min_size(egui::pos2(230., 100.), egui::vec2(130., 200.)),
                                    &[100.],
                                    &[100.]
                                ),
                                &node_props_4.name,
                                preview_texture_4,
                                &mut node_props_4.intensity,
                            ));
                        }
                        {
                            let node_props_5 = if let NodeProps::EffectNode(s) = graph.node_props_mut(&node5_id).unwrap() {s} else {panic!("Not EffectNode!")};
                            ui.add(EffectNodeTile::new(
                                Tile::new(
                                    egui::Rect::from_min_size(egui::pos2(360., 100.), egui::vec2(130., 420.)),
                                    &[100., 320.],
                                    &[210.]
                                ),
                                &node_props_5.name,
                                preview_texture_5,
                                &mut node_props_5.intensity,
                            ));
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
                    egui_rpass.update_texture(&device, &queue, *texture_id, image_delta);
                }
                egui_rpass.update_buffers(&device, &queue, &clipped_primitives, &screen_descriptor);

                // Record all render passes.
                egui_rpass
                    .execute(
                        &mut encoder,
                        &view,
                        &clipped_primitives,
                        &screen_descriptor,
                        Some(wgpu::Color {
                            r: BACKGROUND_COLOR.r() as f64 / 255.,
                            g: BACKGROUND_COLOR.g() as f64 / 255.,
                            b: BACKGROUND_COLOR.b() as f64 / 255.,
                            a: BACKGROUND_COLOR.a() as f64 / 255.,
                        }),
                    );
                // Submit the commands.
                queue.submit(iter::once(encoder.finish()));

                // Draw
                output.present();

                for texture_id in tdelta.free.iter() {
                    egui_rpass.free_texture(texture_id);
                }

                //match ui_renderer.render(&surface, encoder) {
                //    Ok(_) => {}
                //    // Reconfigure the surface if lost
                //    Err(wgpu::SurfaceError::Lost) => {
                //        resize(size, &mut config, &device, &mut surface, &mut ui_renderer);
                //    },
                //    // The system is out of memory, we should probably quit
                //    Err(wgpu::SurfaceError::OutOfMemory) => *control_flow = ControlFlow::Exit,
                //    // All other errors (Outdated, Timeout) should be resolved by the next frame
                //    Err(e) => eprintln!("{:?}", e),
                //}
            }
            Event::MainEventsCleared => {
                // RedrawRequested will only trigger once, unless we manually
                // request it.
                window.request_redraw();
            }
            Event::WindowEvent {
                ref event,
                window_id,
            } if window_id == window.id() => if !false { // XXX
                // Pass the winit events to the EGUI platform integration.
                if platform.on_event(&egui_ctx, &event) {
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
                        size = *physical_size;
                        resize(size, &mut config, &device, &mut surface, &mut screen_descriptor);
                    }
                    WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                        size = **new_inner_size;
                        resize(size, &mut config, &device, &mut surface, &mut screen_descriptor);
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
