use winit::{
    event::*,
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};
use std::sync::Arc;
use serde_json::json;

use radiance::{Context, RenderTargetList, RenderTargetId, Graph, NodeId, ArcTextureViewSampler};

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

// The uniform buffer associated with the node preview
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct VideoNodePreviewUniforms {
    view: [f32; 16],
    pos_min: [f32; 2],
    pos_max: [f32; 2],
    _padding: [u8; 0],
}

pub fn resize(new_size: winit::dpi::PhysicalSize<u32>, config: &mut wgpu::SurfaceConfiguration, device: &wgpu::Device, surface: &mut wgpu::Surface) {
    if new_size.width > 0 && new_size.height > 0 {
        config.width = new_size.width;
        config.height = new_size.height;
        surface.configure(device, config);
    }
}
fn render_screen(device: &wgpu::Device, video_node_preview_pipeline: &wgpu::RenderPipeline, surface: &wgpu::Surface, queue: &wgpu::Queue, mut encoder: wgpu::CommandEncoder, video_node_preview_bind_group_layout: &wgpu::BindGroupLayout, preview_texture: &ArcTextureViewSampler, video_node_preview_uniform_buffer: &wgpu::Buffer) -> Result<(), wgpu::SurfaceError> {
    let output = surface.get_current_texture()?;
    let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

    let video_node_preview_bind_group = device.create_bind_group(
        &wgpu::BindGroupDescriptor {
            layout: &video_node_preview_bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: video_node_preview_uniform_buffer.as_entire_binding(),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::TextureView(&preview_texture.view),
                },
                wgpu::BindGroupEntry {
                    binding: 2,
                    resource: wgpu::BindingResource::Sampler(&preview_texture.sampler),
                },
            ],
            label: Some("preview bind group"),
        }
    );

    {
        let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("Render Pass"),
            color_attachments: &[
                // This is what @location(0) in the fragment shader targets
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

        // NEW!
        render_pass.set_pipeline(video_node_preview_pipeline);
        render_pass.set_bind_group(0, &video_node_preview_bind_group, &[]);
        render_pass.draw(0..4, 0..1);
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

    let video_node_preview_shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
        label: Some("Shader"),
        source: wgpu::ShaderSource::Wgsl(include_str!("video_node_preview.wgsl").into()),
    });

    let video_node_preview_bind_group_layout = device.create_bind_group_layout(
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

    let video_node_preview_pipeline_layout =
        device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("Screen Render Pipeline Layout"),
            bind_group_layouts: &[&video_node_preview_bind_group_layout],
            push_constant_ranges: &[],
        });

    let video_node_preview_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
        label: Some("Screen Render Pipeline"),
        layout: Some(&video_node_preview_pipeline_layout),
        vertex: wgpu::VertexState {
            module: &video_node_preview_shader,
            entry_point: "vs_main",
            buffers: &[],
        },
        fragment: Some(wgpu::FragmentState {
            module: &video_node_preview_shader,
            entry_point: "fs_main",
            targets: &[Some(wgpu::ColorTargetState {
                format: config.format,
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

    // The uniform buffer for the video node preview
    let video_node_preview_uniform_buffer = device.create_buffer(
        &wgpu::BufferDescriptor {
            label: Some("video node preview uniform buffer"),
            size: std::mem::size_of::<VideoNodePreviewUniforms>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        }
    );

    let video_node_preview_uniforms = VideoNodePreviewUniforms {
        view: [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.],
        pos_min: [0., 0.],
        pos_max: [0.5, 0.5],
        ..Default::default()
    };
    queue.write_buffer(&video_node_preview_uniform_buffer, 0, bytemuck::cast_slice(&[video_node_preview_uniforms]));

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

                match render_screen(&device, &video_node_preview_pipeline, &surface, &queue, encoder, &video_node_preview_bind_group_layout, preview_texture, &video_node_preview_uniform_buffer) {
                    Ok(_) => {}
                    // Reconfigure the surface if lost
                    Err(wgpu::SurfaceError::Lost) => {
                        resize(size, &mut config, &device, &mut surface);
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
                        resize(size, &mut config, &device, &mut surface);
                    }
                    WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                        size = **new_inner_size;
                        resize(size, &mut config, &device, &mut surface);
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
