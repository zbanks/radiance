extern crate nalgebra as na;

use winit::{
    event::*,
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};
use std::sync::Arc;
use serde_json::json;
use na::Vector2;

use radiance::{Context, RenderTargetList, RenderTargetId, Graph, NodeId, NodeState, Mir};

mod ui;

pub fn resize(new_size: winit::dpi::PhysicalSize<u32>, config: &mut wgpu::SurfaceConfiguration, device: &wgpu::Device, surface: &mut wgpu::Surface, ui_renderer: &mut ui::Renderer) {
    if new_size.width > 0 && new_size.height > 0 {
        config.width = new_size.width;
        config.height = new_size.height;
        surface.configure(device, config);
        ui_renderer.resize(new_size.width, new_size.height);
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

    // Surface configuration handled by resize()
    let mut ui_renderer = ui::Renderer::new(device.clone(), queue.clone());
    resize(size, &mut config, &device, &mut surface, &mut ui_renderer);

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
    let node5_id: NodeId = NodeId::gen();
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
                let preview_texture_1 = results.get(&node1_id).unwrap();
                let preview_texture_2 = results.get(&node2_id).unwrap();
                let preview_texture_3 = results.get(&node3_id).unwrap();
                let preview_texture_4 = results.get(&node4_id).unwrap();
                let preview_texture_5 = results.get(&node5_id).unwrap();

                // Draw preview
                ui_renderer.effect_node(node_state_1, preview_texture_1, &Vector2::<f32>::new(100., 320.), &Vector2::<f32>::new(230., 520.), &[100.], &[100.]);
                ui_renderer.effect_node(node_state_2, preview_texture_2, &Vector2::<f32>::new(230., 320.), &Vector2::<f32>::new(360., 520.), &[100.], &[100.]);
                ui_renderer.effect_node(node_state_3, preview_texture_3, &Vector2::<f32>::new(100., 100.), &Vector2::<f32>::new(230., 300.), &[100.], &[100.]);
                ui_renderer.effect_node(node_state_4, preview_texture_4, &Vector2::<f32>::new(230., 100.), &Vector2::<f32>::new(360., 300.), &[100.], &[100.]);
                ui_renderer.effect_node(node_state_5, preview_texture_5, &Vector2::<f32>::new(360., 100.), &Vector2::<f32>::new(490., 520.), &[100., 320.], &[210.]);

                match ui_renderer.render(&surface, encoder) {
                    Ok(_) => {}
                    // Reconfigure the surface if lost
                    Err(wgpu::SurfaceError::Lost) => {
                        resize(size, &mut config, &device, &mut surface, &mut ui_renderer);
                    },
                    // The system is out of memory, we should probably quit
                    Err(wgpu::SurfaceError::OutOfMemory) => *control_flow = ControlFlow::Exit,
                    // All other errors (Outdated, Timeout) should be resolved by the next frame
                    Err(e) => eprintln!("{:?}", e),
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
                        resize(size, &mut config, &device, &mut surface, &mut ui_renderer);
                    }
                    WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                        size = **new_inner_size;
                        resize(size, &mut config, &device, &mut surface, &mut ui_renderer);
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
