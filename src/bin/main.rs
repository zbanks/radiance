// For profiling (see also Cargo.toml)
//use jemallocator::Jemalloc;
//
//#[global_allocator]
//static GLOBAL: Jemalloc = Jemalloc;

extern crate nalgebra as na;

use egui_wgpu::renderer::{Renderer, ScreenDescriptor};
use egui_winit::winit;
use egui_winit::winit::{
    event::*,
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};
use serde_json::json;
use std::collections::HashMap;
use std::fs::{read_to_string, File};
use std::io::Write;
use std::iter;
use std::sync::Arc;

use radiance::{
    AutoDJ, Context, EffectNodeProps, ImageNodeProps, InsertionPoint, Mir, MovieNodeProps, NodeId,
    NodeProps, ProjectionMappedOutputNodeProps, Props, RenderTarget, RenderTargetId,
    ScreenOutputNodeProps,
};

mod ui;
use ui::{modal, modal_shown, mosaic};
use ui::{BeatWidget, SpectrumWidget, WaveformWidget};

mod winit_output;
use winit_output::WinitOutput;

const AUTOSAVE_INTERVAL_FRAMES: usize = 60 * 10;
const AUTOSAVE_FILENAME: &str = "radiance_autosave.json";

const BACKGROUND_COLOR: egui::Color32 = egui::Color32::from_rgb(51, 51, 51);

pub fn resize(
    new_size: winit::dpi::PhysicalSize<u32>,
    config: &mut wgpu::SurfaceConfiguration,
    device: &wgpu::Device,
    surface: &mut wgpu::Surface,
    screen_descriptor: Option<&mut ScreenDescriptor>,
) {
    if new_size.width > 0 && new_size.height > 0 {
        config.width = new_size.width;
        config.height = new_size.height;
        surface.configure(device, config);
        if let Some(screen_descriptor) = screen_descriptor {
            screen_descriptor.size_in_pixels = [config.width, config.height]
        }
    }
}

fn autosave(props: &Props) {
    fn inner(props: &Props) -> Result<(), String> {
        let contents = serde_json::to_string(props).map_err(|e| format!("{:?}", e))?;
        let mut file = File::create(AUTOSAVE_FILENAME).map_err(|e| format!("{:?}", e))?;
        file.write_all(contents.as_bytes())
            .map_err(|e| format!("{:?}", e))?;
        Ok(())
    }

    match inner(&props) {
        Ok(_) => {}
        Err(msg) => {
            println!("Failed to write autosave file: {}", msg)
        }
    };
}

pub async fn run() {
    env_logger::init();
    let event_loop = EventLoop::new();

    let window = WindowBuilder::new().build(&event_loop).unwrap();
    window.set_title("Radiance");
    window.set_maximized(true);

    let size = window.inner_size();

    // The instance is a handle to our GPU
    // Backends::all => Vulkan + Metal + DX12 + Browser WebGPU
    let instance = Arc::new(wgpu::Instance::new(wgpu::Backends::all()));
    let mut surface = unsafe { instance.create_surface(&window) };
    let adapter = Arc::new(
        instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::default(),
                compatible_surface: Some(&surface),
                force_fallback_adapter: false,
            })
            .await
            .unwrap(),
    );

    let (device, queue) = adapter
        .request_device(
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
        )
        .await
        .unwrap();

    let device = Arc::new(device);
    let queue = Arc::new(queue);

    let mut winit_output = WinitOutput::new(
        instance.clone(),
        adapter.clone(),
        device.clone(),
        queue.clone(),
    );

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

    resize(
        size,
        &mut config,
        &device,
        &mut surface,
        Some(&mut screen_descriptor),
    );

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

    // Make widgets
    let mut waveform_widget = WaveformWidget::new(device.clone(), queue.clone(), pixels_per_point);
    let mut spectrum_widget = SpectrumWidget::new(device.clone(), queue.clone(), pixels_per_point);
    let mut beat_widget = BeatWidget::new(device.clone(), queue.clone(), pixels_per_point);

    // Make an AutoDJ
    let mut auto_dj_1: Option<AutoDJ> = None;
    let mut auto_dj_2: Option<AutoDJ> = None;

    fn read_autosave_file() -> Result<Props, String> {
        let contents = read_to_string(AUTOSAVE_FILENAME).map_err(|e| format!("{:?}", e))?;
        serde_json::from_str(contents.as_str()).map_err(|e| format!("{:?}", e))
    }

    let mut props = read_autosave_file().unwrap_or_else(|err_string| {
        println!("Failed to read autosave file ({})", err_string);

        // Make a graph
        let node1_id: NodeId =
            serde_json::from_value(json!("node_TW+qCFNoz81wTMca9jRIBg")).unwrap();
        let node2_id: NodeId =
            serde_json::from_value(json!("node_IjPuN2HID3ydxcd4qOsCuQ")).unwrap();
        let node3_id: NodeId =
            serde_json::from_value(json!("node_mW00lTCmDH/03tGyNv3iCQ")).unwrap();
        let node4_id: NodeId =
            serde_json::from_value(json!("node_EdpVLI4KG5JEBRNSgKUzsw")).unwrap();
        let node5_id: NodeId =
            serde_json::from_value(json!("node_I6AAXBaZKvSUfArs2vBr4A")).unwrap();
        let node6_id: NodeId =
            serde_json::from_value(json!("node_I6AAXBaZKvSUfAxs2vBr4A")).unwrap();
        let output_node_id: NodeId =
            serde_json::from_value(json!("node_KSvPLGkiJDT+3FvPLf9JYQ")).unwrap();
        serde_json::from_value(json!({
            "graph": {
                "nodes": [
                    node1_id,
                    node2_id,
                    node3_id,
                    node4_id,
                    node5_id,
                    node6_id,
                    output_node_id,
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
                        "to": output_node_id,
                        "input": 0,
                    },
                    {
                        "from": node6_id,
                        "to": node1_id,
                        "input": 0,
                    },
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
                node6_id.to_string(): {
                    "type": "ImageNode",
                    "name": "nyancat.gif",
                    "intensity": 1.0,
                },
                output_node_id.to_string(): {
                    "type": "ProjectionMappedOutputNode",
                    "resolution": [1000, 1000],
                    "screens": [
                        {
                            "name": "HDMI-0",
                            "resolution": [1920, 1080],
                            "crop": [[0.2,0.8], [0.8,0.8], [0.8, 0.3], [0.5, 0.2], [0.2, 0.5]],
                            "map": [1, 0.2, 0, -0.2, 1, 0, 0, 0, 1],
                        },
                        //{
                        //    "name": "fake2",
                        //    "resolution": [1920, 1080],
                        //    "crop": [[0.2,0.8], [0.8,0.8], [0.8, 0.3], [0.5, 0.2], [0.2, 0.5]],
                        //    "map": [1.5, 0.2, 0, -0.2, 1.5, 0, 0, 0, 1],
                        //},
                    ],
                }
            },
            "time": 0.,
            "dt": 0.03,
        }))
        .unwrap()
    });

    println!("Props: {}", serde_json::to_string(&props).unwrap());

    // Make render targets
    let preview_render_target_id: RenderTargetId =
        serde_json::from_value(json!("rt_LVrjzxhXrGU7SqFo+85zkw")).unwrap();
    let render_target_list: HashMap<RenderTargetId, RenderTarget> = serde_json::from_value(json!({
        preview_render_target_id.to_string(): {
            "width": 256,
            "height": 256,
            "dt": 1. / 60.
        },
    }))
    .unwrap();

    println!(
        "Render target list: {}",
        serde_json::to_string(&render_target_list).unwrap()
    );

    // UI state
    let mut node_add_textedit = String::new(); // TODO: factor this into its own component in ui/
    let mut left_panel_expanded = false;
    let mut node_add_wants_focus = false;
    let mut insertion_point: InsertionPoint = Default::default();
    let mut auto_dj_1_enabled = false;
    let mut auto_dj_2_enabled = false;

    let mut waveform_texture: Option<egui::TextureId> = None;
    let mut spectrum_texture: Option<egui::TextureId> = None;
    let mut beat_texture: Option<egui::TextureId> = None;

    let mut autosave_timer: usize = 0;

    event_loop.run(move |event, event_loop, control_flow| {
        if winit_output.on_event(&event, &event_loop, &mut ctx) {
            return; // Event was consumed by winit_output
        }

        match event {
            Event::RedrawRequested(window_id) if window_id == window.id() => {
                // Update
                let music_info = mir.poll();
                props.time = music_info.time;
                props.dt = music_info.tempo * (1. / 60.);
                props.audio = music_info.audio.clone();
                // Merge our render list and the winit_output render list into one:
                let render_target_list = render_target_list
                    .iter()
                    .chain(winit_output.render_targets_iter())
                    .map(|(k, v)| (*k, v.clone()))
                    .collect();
                winit_output.update(event_loop, &mut props);
                auto_dj_1.as_mut().map(|a| {
                    a.update(&mut props);

                    // Uncheck the checkbox if we broke the AutoDJ
                    if a.is_broken() {
                        auto_dj_1_enabled = false;
                    }
                });
                auto_dj_2.as_mut().map(|a| {
                    a.update(&mut props);

                    // Uncheck the checkbox if we broke the AutoDJ
                    if a.is_broken() {
                        auto_dj_2_enabled = false;
                    }
                });

                ctx.update(&mut props, &render_target_list);

                // Autosave if necessary
                // TODO: consider moving this to a background thread
                if autosave_timer == 0 {
                    autosave(&props);
                    autosave_timer = AUTOSAVE_INTERVAL_FRAMES;
                } else {
                    autosave_timer -= 1;
                }

                // Paint
                let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("Encoder"),
                });

                let results = ctx.paint(&mut encoder, preview_render_target_id);

                let preview_images: HashMap<NodeId, egui::TextureId> = props
                    .graph
                    .nodes
                    .iter()
                    .map(|&node_id| {
                        let tex_id = egui_renderer.register_native_texture(
                            &device,
                            &results.get(&node_id).unwrap().view,
                            wgpu::FilterMode::Linear,
                        );
                        (node_id, tex_id)
                    })
                    .collect();

                // Update & paint widgets

                fn update_or_register_native_texture(
                    egui_renderer: &mut egui_wgpu::Renderer,
                    device: &wgpu::Device,
                    native_texture: &wgpu::TextureView,
                    egui_texture: &mut Option<egui::TextureId>,
                ) {
                    match egui_texture {
                        None => {
                            *egui_texture = Some(egui_renderer.register_native_texture(
                                device,
                                native_texture,
                                wgpu::FilterMode::Linear,
                            ));
                        }
                        Some(egui_texture) => {
                            egui_renderer.update_egui_texture_from_wgpu_texture(
                                device,
                                native_texture,
                                wgpu::FilterMode::Linear,
                                *egui_texture,
                            );
                        }
                    }
                }

                let waveform_size = egui::vec2(330., 65.);
                let waveform_native_texture = waveform_widget.paint(
                    waveform_size,
                    &music_info.audio,
                    music_info.uncompensated_unscaled_time,
                );

                update_or_register_native_texture(
                    &mut egui_renderer,
                    &device,
                    &waveform_native_texture.view,
                    &mut waveform_texture,
                );

                let spectrum_size = egui::vec2(330., 65.);
                let spectrum_native_texture =
                    spectrum_widget.paint(spectrum_size, &music_info.spectrum);

                update_or_register_native_texture(
                    &mut egui_renderer,
                    &device,
                    &spectrum_native_texture.view,
                    &mut spectrum_texture,
                );

                let beat_size = egui::vec2(65., 65.);
                let beat_native_texture = beat_widget.paint(beat_size, music_info.unscaled_time);

                update_or_register_native_texture(
                    &mut egui_renderer,
                    &device,
                    &beat_native_texture.view,
                    &mut beat_texture,
                );

                // EGUI update
                let raw_input = platform.take_egui_input(&window);
                let full_output = egui_ctx.run(raw_input, |egui_ctx| {
                    let left_panel_response = egui::SidePanel::left("left").show_animated(
                        egui_ctx,
                        left_panel_expanded,
                        |ui| ui.text_edit_singleline(&mut node_add_textedit),
                    );

                    let full_rect = egui_ctx.available_rect();
                    egui::CentralPanel::default().show(egui_ctx, |ui| {
                        let modal_id = ui.make_persistent_id("modal");
                        let modal_shown = modal_shown(egui_ctx, modal_id);

                        ui.allocate_ui_at_rect(full_rect, |ui| {
                            ui.set_enabled(!modal_shown);
                            ui.horizontal(|ui| {
                                ui.image(waveform_texture.unwrap(), waveform_size);
                                ui.image(spectrum_texture.unwrap(), spectrum_size);
                                ui.image(beat_texture.unwrap(), beat_size);
                                ui.checkbox(&mut auto_dj_1_enabled, "Auto DJ 1");
                                ui.checkbox(&mut auto_dj_2_enabled, "Auto DJ 2");

                                ui.label("Global timescale:");
                                let timescales: &[f32] = &[0.125, 0.25, 0.5, 1., 2., 4., 8.];
                                fn str_for_timescale(timescale: f32) -> String {
                                    if timescale < 1. {
                                        format!("{}x slower", 1. / timescale)
                                    } else if timescale == 1. {
                                        "1x".to_owned()
                                    } else if timescale > 1. {
                                        format!("{}x faster", timescale)
                                    } else {
                                        format!("{}", timescale)
                                    }
                                }
                                egui::ComboBox::from_id_source("global timescale")
                                    .selected_text(str_for_timescale(mir.global_timescale).as_str())
                                    .show_ui(ui, |ui| {
                                        for &timescale in timescales.iter() {
                                            ui.selectable_value(
                                                &mut mir.global_timescale,
                                                timescale,
                                                str_for_timescale(timescale).as_str(),
                                            );
                                        }
                                    });
                                ui.label("Latency compensation:");
                                ui.add(
                                    egui::DragValue::new(&mut mir.latency_compensation)
                                        .speed(0.001)
                                        .fixed_decimals(3)
                                        .suffix("s")
                                        .clamp_range(0. ..=1.),
                                );
                            });

                            let mosaic_response = ui.add(mosaic(
                                "mosaic",
                                &mut props,
                                ctx.node_states(),
                                &preview_images,
                                &mut insertion_point,
                                modal_id,
                            ));

                            if !left_panel_expanded && ui.input().key_pressed(egui::Key::A) {
                                left_panel_expanded = true;
                                node_add_wants_focus = true;
                            }

                            if let Some(egui::InnerResponse {
                                inner: node_add_response,
                                response: _,
                            }) = left_panel_response
                            {
                                // TODO all this side-panel handling is wonky. It is done, in part, to avoid mutating the props before it's drawn.
                                // This needs to be factored out into a real "library" component.
                                if node_add_wants_focus {
                                    node_add_response.request_focus();
                                    node_add_wants_focus = false;
                                }
                                if node_add_response.lost_focus() {
                                    if egui_ctx.input().key_pressed(egui::Key::Enter) {
                                        let node_add_textedit_str = node_add_textedit.as_str();
                                        if node_add_textedit_str.starts_with("http:")
                                            || node_add_textedit_str.starts_with("https:")
                                            || node_add_textedit_str.ends_with(".mp4")
                                            || node_add_textedit_str.ends_with(".mkv")
                                            || node_add_textedit_str.ends_with(".avi")
                                        {
                                            let new_node_id = NodeId::gen();
                                            let new_node_props =
                                                NodeProps::MovieNode(MovieNodeProps {
                                                    name: node_add_textedit.clone(),
                                                    ..Default::default()
                                                });
                                            props.node_props.insert(new_node_id, new_node_props);
                                            props.graph.insert_node(new_node_id, &insertion_point);
                                        } else if node_add_textedit_str.ends_with(".png")
                                            || node_add_textedit_str.starts_with(".jpg")
                                            || node_add_textedit_str.ends_with(".gif")
                                        {
                                            let new_node_id = NodeId::gen();
                                            let new_node_props =
                                                NodeProps::ImageNode(ImageNodeProps {
                                                    name: node_add_textedit.clone(),
                                                    ..Default::default()
                                                });
                                            props.node_props.insert(new_node_id, new_node_props);
                                            props.graph.insert_node(new_node_id, &insertion_point);
                                        } else {
                                            match node_add_textedit.as_str() {
                                                "ScreenOutput" => {
                                                    let new_node_id = NodeId::gen();
                                                    let new_node_props =
                                                        NodeProps::ScreenOutputNode(
                                                            ScreenOutputNodeProps {
                                                                ..Default::default()
                                                            },
                                                        );
                                                    props
                                                        .node_props
                                                        .insert(new_node_id, new_node_props);
                                                    props
                                                        .graph
                                                        .insert_node(new_node_id, &insertion_point);
                                                }
                                                "ProjectionMappedOutput" => {
                                                    let new_node_id = NodeId::gen();
                                                    let new_node_props =
                                                        NodeProps::ProjectionMappedOutputNode(
                                                            ProjectionMappedOutputNodeProps {
                                                                ..Default::default()
                                                            },
                                                        );
                                                    props
                                                        .node_props
                                                        .insert(new_node_id, new_node_props);
                                                    props
                                                        .graph
                                                        .insert_node(new_node_id, &insertion_point);
                                                }
                                                _ => {
                                                    let new_node_id = NodeId::gen();
                                                    let new_node_props =
                                                        NodeProps::EffectNode(EffectNodeProps {
                                                            name: node_add_textedit.clone(),
                                                            ..Default::default()
                                                        });
                                                    props
                                                        .node_props
                                                        .insert(new_node_id, new_node_props);
                                                    props
                                                        .graph
                                                        .insert_node(new_node_id, &insertion_point);
                                                    // TODO: select and focus the new node
                                                    // (consider making selection & focus part of the explicit state of mosaic, not memory)
                                                }
                                            }
                                        }
                                    }
                                    node_add_textedit.clear();
                                    left_panel_expanded = false;
                                    mosaic_response.request_focus();
                                }
                            }
                        });

                        if modal_shown {
                            ui.allocate_ui_at_rect(full_rect, |ui| {
                                ui.add(modal(
                                    modal_id,
                                    &mut props,
                                    ctx.node_states(),
                                    &preview_images,
                                ));
                            });
                        }
                    });
                });

                // Construct or destroy the AutoDJs
                match (auto_dj_1_enabled, &mut auto_dj_1) {
                    (false, Some(_)) => {
                        auto_dj_1 = None;
                    }
                    (true, None) => {
                        auto_dj_1 = Some(AutoDJ::new());
                    }
                    _ => {}
                }
                match (auto_dj_2_enabled, &mut auto_dj_2) {
                    (false, Some(_)) => {
                        auto_dj_2 = None;
                    }
                    (true, None) => {
                        auto_dj_2 = Some(AutoDJ::new());
                    }
                    _ => {}
                }

                platform.handle_platform_output(&window, &egui_ctx, full_output.platform_output);
                let clipped_primitives = egui_ctx.tessellate(full_output.shapes); // create triangles to paint

                // EGUI paint
                let output = surface.get_current_texture().unwrap();
                let view = output
                    .texture
                    .create_view(&wgpu::TextureViewDescriptor::default());

                // Upload all resources for the GPU.
                let tdelta: egui::TexturesDelta = full_output.textures_delta;
                for (texture_id, image_delta) in tdelta.set.iter() {
                    egui_renderer.update_texture(&device, &queue, *texture_id, image_delta);
                }
                egui_renderer.update_buffers(
                    &device,
                    &queue,
                    &mut encoder,
                    &clipped_primitives,
                    &screen_descriptor,
                );

                // Record UI render pass.
                {
                    let mut egui_render_pass =
                        encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
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

                    egui_renderer.render(
                        &mut egui_render_pass,
                        &clipped_primitives,
                        &screen_descriptor,
                    );
                }

                // Submit the commands.
                queue.submit(iter::once(encoder.finish()));

                // Draw
                output.present();

                // Clear out all native textures for the next frame
                for texture_id in preview_images.values() {
                    egui_renderer.free_texture(texture_id);
                }

                // Clear out egui textures for the next frame
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
            } if window_id == window.id() => {
                if true {
                    // XXX
                    // Pass the winit events to the EGUI platform integration.
                    if platform.on_event(&egui_ctx, event).consumed {
                        return; // EGUI wants exclusive use of this event
                    }
                    match event {
                        WindowEvent::CloseRequested => {
                            autosave(&props);
                            *control_flow = ControlFlow::Exit;
                        }
                        WindowEvent::Resized(physical_size) => {
                            let size = *physical_size;
                            resize(
                                size,
                                &mut config,
                                &device,
                                &mut surface,
                                Some(&mut screen_descriptor),
                            );
                        }
                        WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                            let size = **new_inner_size;
                            resize(
                                size,
                                &mut config,
                                &device,
                                &mut surface,
                                Some(&mut screen_descriptor),
                            );
                        }
                        _ => {}
                    }
                }
            }
            _ => {}
        }
    });
}

pub fn main() {
    pollster::block_on(run());
}
