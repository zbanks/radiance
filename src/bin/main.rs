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

use radiance::{Context, RenderTarget, RenderTargetId, Props, NodeId, Mir, NodeProps, EffectNodeProps, InsertionPoint, ScreenOutputNodeProps};

mod ui;
use ui::{mosaic};

mod winit_output;
use winit_output::WinitOutput;

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
    let size = window.inner_size();

    // The instance is a handle to our GPU
    // Backends::all => Vulkan + Metal + DX12 + Browser WebGPU
    let instance = Arc::new(wgpu::Instance::new(wgpu::Backends::all()));
    let mut surface = unsafe { instance.create_surface(&window) };
    let adapter = Arc::new(instance.request_adapter(
        &wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::default(),
            compatible_surface: Some(&surface),
            force_fallback_adapter: false,
        },
    ).await.unwrap());

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

    let mut winit_output = WinitOutput::new(instance.clone(), adapter.clone(), device.clone(), queue.clone());

    let mut config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format: surface.get_supported_formats(&adapter)[0],
        width: size.width,
        height: size.height,
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

    // Make a egui context:
    let egui_ctx = egui::Context::default();

    // We use the egui_winit_platform crate as the platform.
    let mut platform = egui_winit::State::new(&event_loop);
    platform.set_pixels_per_point(pixels_per_point);

    // We use the egui_wgpu_backend crate as the render backend.
    let mut egui_renderer = Renderer::new(&device, config.format, None, 1);

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

    // Make render targets
    let preview_render_target_id: RenderTargetId = serde_json::from_value(json!("rt_LVrjzxhXrGU7SqFo+85zkw")).unwrap();
    let render_target_list: HashMap<RenderTargetId, RenderTarget> = serde_json::from_value(json!({
        preview_render_target_id.to_string(): {
            "width": 256,
            "height": 256,
            "dt": 1. / 60.
        },
    })).unwrap();

    println!("Render target list: {}", serde_json::to_string(&render_target_list).unwrap());

    // UI state
    let mut node_add_textedit = String::new(); // TODO: factor this into its own component in ui/
    let mut left_panel_expanded = false;
    let mut node_add_wants_focus = false;
    let mut insertion_point: InsertionPoint = Default::default();

    event_loop.run(move |event, event_loop, control_flow| {

        if winit_output.on_event(&event, &mut ctx) {
            return; // Event was consumed by winit_output
        }

        match event {
            Event::RedrawRequested(window_id) if window_id == window.id() => {
                // Update
                let music_info = mir.poll();
                props.time = music_info.time;
                props.dt = music_info.tempo * (1. / 60.);
                // Merge our render list and the winit_output render list into one:
                let render_target_list = render_target_list.iter().chain(winit_output.render_targets_iter()).map(|(k, v)| (k.clone(), v.clone())).collect();
                ctx.update(&mut props, &render_target_list);
                winit_output.update(event_loop, &mut props);

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
                                    match node_add_textedit.as_str() {
                                        "ScreenOutput" => {
                                            let new_node_id = NodeId::gen();
                                            let new_node_props = NodeProps::ScreenOutputNode(ScreenOutputNodeProps {
                                                ..Default::default()
                                            });
                                            props.node_props.insert(new_node_id, new_node_props);
                                            props.graph.insert_node(new_node_id, &insertion_point);
                                        },
                                        _ => {
                                            let new_node_id = NodeId::gen();
                                            let new_node_props = NodeProps::EffectNode(EffectNodeProps {
                                                name: node_add_textedit.clone(),
                                                ..Default::default()
                                            });
                                            props.node_props.insert(new_node_id, new_node_props);
                                            props.graph.insert_node(new_node_id, &insertion_point);
                                            // TODO: select and focus the new node
                                            // (consider making selection & focus part of the explicit state of mosaic, not memory)
                                        },
                                    }
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
                if platform.on_event(&egui_ctx, event).consumed {
                    return; // EGUI wants exclusive use of this event
                }
                match event {
                    WindowEvent::CloseRequested => {
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
            _ => {}
        }
    });
}

pub fn main() {
    pollster::block_on(run());
}
