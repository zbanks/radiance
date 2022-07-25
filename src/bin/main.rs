extern crate nalgebra as na;

use winit::{
    event::*,
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};
use std::sync::Arc;
use serde_json::json;
use na::{Vector2, Matrix4};

use radiance::{Context, RenderTargetList, RenderTargetId, Graph, NodeId, ArcTextureViewSampler};

mod ui;

const PIXEL_SCALING: f32 = 2.;

///// A vertex passed to the video_node_decoration.wgsl vertex shader
//#[repr(C)]
//#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
//struct VideoNodeDecorationVertex {
//    pos: [f32; 2],
//    color: [f32; 4],
//    _padding: [u8; 2],
//}
//
//impl VideoNodeDecorationVertex {
//    fn desc<'a>() -> wgpu::VertexBufferLayout<'a> {
//        use std::mem;
//        wgpu::VertexBufferLayout {
//            array_stride: mem::size_of::<Vertex>() as wgpu::BufferAddress,
//            step_mode: wgpu::VertexStepMode::Vertex,
//            attributes: &[
//                wgpu::VertexAttribute {
//                    offset: 0,
//                    shader_location: 0,
//                    format: wgpu::VertexFormat::Float32x2,
//                },
//                wgpu::VertexAttribute {
//                    offset: mem::size_of::<[f32; 2]>() as wgpu::BufferAddress,
//                    shader_location: 1,
//                    format: wgpu::VertexFormat::Float32x4,
//                },
//            ],
//        }
//    }
//}

fn view_matrix(width: u32, height: u32) -> Matrix4<f32> {
    let m = Matrix4::<f32>::new(
        2.0 / width as f32, 0., 0., -1.,
        0., -2.0 / height as f32, 0., 1.,
        0., 0., 1., 0.,
        0., 0., 0., 1.
    );

    let m = m * Matrix4::<f32>::new(
        PIXEL_SCALING, 0., 0., 0.,
        0., PIXEL_SCALING, 0., 0.,
        0., 0., 1., 0.,
        0., 0., 0., 1.
    );
    m
}

pub fn resize(new_size: winit::dpi::PhysicalSize<u32>, config: &mut wgpu::SurfaceConfiguration, device: &wgpu::Device, surface: &mut wgpu::Surface, video_node_preview_renderer: &mut ui::VideoNodePreviewRenderer, video_node_tile_renderer: &mut ui::VideoNodeTileRenderer) {
    if new_size.width > 0 && new_size.height > 0 {
        config.width = new_size.width;
        config.height = new_size.height;
        surface.configure(device, config);
        video_node_preview_renderer.set_view(&view_matrix(new_size.width, new_size.height));
        video_node_tile_renderer.set_view(&view_matrix(new_size.width, new_size.height));
    }
}
fn render_screen(device: &wgpu::Device, surface: &wgpu::Surface, queue: &wgpu::Queue, mut encoder: wgpu::CommandEncoder, video_node_preview_renderer: &mut ui::VideoNodePreviewRenderer, video_node_tile_renderer: &mut ui::VideoNodeTileRenderer) -> Result<(), wgpu::SurfaceError> {
    let output = surface.get_current_texture()?;
    let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

    let video_node_preview_resources = video_node_preview_renderer.prepare();
    let video_node_tile_resources = video_node_tile_renderer.prepare();

    {
        let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("Render Pass"),
            color_attachments: &[
                Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(
                            wgpu::Color {
                                r: 0.1,
                                g: 0.2,
                                b: 0.3,
                                a: 1.0,
                            }
                        ),
                        store: true,
                    }
                })
            ],
            depth_stencil_attachment: None,
        });

        video_node_preview_renderer.paint(&mut render_pass, &video_node_preview_resources);
        video_node_tile_renderer.paint(&mut render_pass, &video_node_tile_resources);
    }

    queue.submit(std::iter::once(encoder.finish()));
    output.present();
    Ok(())
}

/*

fn render_texture(device: &wgpu::Device, texture_render_pipeline: &wgpu::RenderPipeline, view: &wgpu::TextureView, queue: &wgpu::Queue) {
    let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
        label: Some("Render Encoder"),
    });

    {
        let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("Render Pass"),
            color_attachments: &[
                // This is what @location(0) in the fragment shader targets
                Some(wgpu::RenderPassColorAttachment {
                    view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(
                            wgpu::Color {
                                r: 0.1,
                                g: 0.2,
                                b: 0.3,
                                a: 1.0,
                            }
                        ),
                        store: true,
                    }
                })
            ],
            depth_stencil_attachment: None,
        });

        // NEW!
        render_pass.set_pipeline(texture_render_pipeline); // 2.
        render_pass.draw(0..3, 0..1); // 3.
    }
    queue.submit(std::iter::once(encoder.finish()));
}
*/

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
    surface.configure(&device, &config);

/*
    let texture1_size = 256u32;

    let texture1_desc = wgpu::TextureDescriptor {
        size: wgpu::Extent3d {
            width: texture1_size,
            height: texture1_size,
            depth_or_array_layers: 1,
        },
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format: wgpu::TextureFormat::Rgba8UnormSrgb,
        usage: wgpu::TextureUsages::COPY_SRC
            | wgpu::TextureUsages::RENDER_ATTACHMENT
            ,
        label: None,
    };
    let texture1 = device.create_texture(&texture1_desc);
    let texture1_view = texture1.create_view(&Default::default());

    let device_thread1 = device.clone();
    let queue_thread1 = queue.clone();
*/

    let mut video_node_preview_renderer = ui::VideoNodePreviewRenderer::new(device.clone(), queue.clone());
    let mut video_node_tile_renderer = ui::VideoNodeTileRenderer::new(device.clone(), queue.clone());
    video_node_preview_renderer.set_view(&view_matrix(size.width, size.height));
    video_node_tile_renderer.set_view(&view_matrix(size.width, size.height));

    // RADIANCE, WOO

    // Make context
    let mut ctx = Context::new(device.clone(), queue.clone());

    // Make a graph
    let node1_id: NodeId = serde_json::from_value(json!("node_TW+qCFNoz81wTMca9jRIBg")).unwrap();
    let node2_id: NodeId = serde_json::from_value(json!("node_IjPuN2HID3ydxcd4qOsCuQ")).unwrap();
    let node3_id: NodeId = serde_json::from_value(json!("node_mW00lTCmDH/03tGyNv3iCQ")).unwrap();
    let mut graph: Graph = serde_json::from_value(json!({
        "nodes": [
            node1_id,
            node2_id,
            node3_id,
        ],
        "edges": [
            {
                "from": node1_id,
                "to": node2_id,
                "input": 0,
            },
            {
                "from": node2_id,
                "to": node3_id,
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
                "name": "droste.wgsl",
                "intensity": 0.0,
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

    let mut a = -10.;

    event_loop.run(move |event, _, control_flow| {
        match event {
            Event::RedrawRequested(window_id) if window_id == window.id() => {
                // Update
                ctx.update(&mut graph, &render_target_list);

                // Paint

                let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Encoder"),
                });

                let results = ctx.paint(&mut encoder, preview_render_target_id);

                // Get node
                let preview_texture = results.get(&node3_id).unwrap();

                // Draw preview
                video_node_preview_renderer.push_instance(preview_texture, &Vector2::<f32>::new(100., 100.), &Vector2::<f32>::new(200., 200.));

                // Draw random thing
                video_node_tile_renderer.push_instance(&Vector2::<f32>::new(300., 300.), &Vector2::<f32>::new(500., 500.), &[100.], &[]);
                video_node_tile_renderer.push_instance(&Vector2::<f32>::new(600., 300.), &Vector2::<f32>::new(700., 500.), &[a, 120.], &[100.]);
                a += 0.1;

                match render_screen(&device, &surface, &queue, encoder, &mut video_node_preview_renderer, &mut video_node_tile_renderer) {
                    Ok(_) => {}
                    // Reconfigure the surface if lost
                    Err(wgpu::SurfaceError::Lost) => {
                        resize(size, &mut config, &device, &mut surface, &mut video_node_preview_renderer, &mut video_node_tile_renderer);
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
                    } => *control_flow = ControlFlow::Exit,
                    WindowEvent::Resized(physical_size) => {
                        size = *physical_size;
                        resize(size, &mut config, &device, &mut surface, &mut video_node_preview_renderer, &mut video_node_tile_renderer);
                    }
                    WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                        size = **new_inner_size;
                        resize(size, &mut config, &device, &mut surface, &mut video_node_preview_renderer, &mut video_node_tile_renderer);
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
