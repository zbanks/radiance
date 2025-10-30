// For profiling (see also Cargo.toml)
//use jemallocator::Jemalloc;
//
//#[global_allocator]
//static GLOBAL: Jemalloc = Jemalloc;

extern crate nalgebra as na;

use eframe::{egui, egui_wgpu};
use serde_json::json;
use std::collections::hash_map::Entry;
use std::collections::HashMap;
use std::fs::{self, read_to_string, File};
use std::io::Write;
use std::iter;
use std::path::Path;

#[cfg(feature = "mpv")]
use radiance::MovieNodeProps;
use radiance::{
    AutoDJ, Context, EffectNodeProps, ImageNodeProps, InsertionPoint, Mir, NodeId, NodeProps,
    ProjectionMappedOutputNodeProps, Props, RenderTarget, RenderTargetId, ScreenOutputNodeProps,
};

mod ui;
use ui::{modal, modal_shown, mosaic};
use ui::{BeatWidget, SpectrumWidget, WaveformWidget};

mod setup;
use setup::load_default_library;

mod winit_output;

const AUTOSAVE_INTERVAL_FRAMES: usize = 60 * 10;
const AUTOSAVE_FILENAME: &str = "autosave.json";

fn autosave(resource_dir: &Path, props: &Props) {
    let inner = || {
        let contents = serde_json::to_string(props).map_err(|e| format!("{:?}", e))?;
        let mut file =
            File::create(resource_dir.join(AUTOSAVE_FILENAME)).map_err(|e| format!("{:?}", e))?;
        file.write_all(contents.as_bytes())
            .map_err(|e| format!("{:?}", e))?;
        Ok(())
    };

    inner().unwrap_or_else(|msg: String| println!("Failed to write autosave file: {}", msg));
}

fn main() -> eframe::Result {
    env_logger::init();

    let instance = wgpu::Instance::new(&wgpu::InstanceDescriptor {
        backends: wgpu::Backends::all(),
        flags: wgpu::InstanceFlags::from_env_or_default(),
        memory_budget_thresholds: Default::default(),
        backend_options: wgpu::BackendOptions::from_env_or_default(),
    });
    let adapter = pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::default(),
        compatible_surface: None,
        force_fallback_adapter: false,
    }))
    .unwrap();

    let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor {
        required_features: wgpu::Features::TEXTURE_BINDING_ARRAY,
        // WebGL doesn't support all of wgpu's features, so if
        // we're building for the web we'll have to disable some.
        required_limits: if cfg!(target_arch = "wasm32") {
            wgpu::Limits::downlevel_webgl2_defaults()
        } else {
            wgpu::Limits {
                max_binding_array_elements_per_shader_stage: 500_000,
                ..Default::default()
            }
        },
        label: None,
        memory_hints: Default::default(),
        trace: wgpu::Trace::Off,
        experimental_features: Default::default(),
    }))
    .unwrap();

    let native_options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title("Radiance")
            .with_maximized(true),
        wgpu_options: egui_wgpu::WgpuConfiguration {
            wgpu_setup: egui_wgpu::WgpuSetup::Existing(egui_wgpu::WgpuSetupExisting {
                instance,
                adapter,
                device,
                queue,
            }),
            ..Default::default()
        },
        ..Default::default()
    };
    eframe::run_native(
        "Radiance",
        native_options,
        Box::new(|cc| Ok(Box::new(App::new(cc)))),
    )
}

struct App {
    mir: Mir,
    ctx: Context,
    props: Props,
    render_target_list: HashMap<RenderTargetId, RenderTarget>,
    auto_dj_1: Option<AutoDJ>,
    auto_dj_2: Option<AutoDJ>,
    auto_dj_1_enabled: bool,
    auto_dj_2_enabled: bool,
    autosave_timer: usize,
    preview_render_target_id: RenderTargetId,
    waveform_widget: WaveformWidget,
    spectrum_widget: SpectrumWidget,
    beat_widget: BeatWidget,
    waveform_texture: Option<egui::TextureId>,
    spectrum_texture: Option<egui::TextureId>,
    beat_texture: Option<egui::TextureId>,
    node_add_textedit: String,
    left_panel_expanded: bool,
    node_add_wants_focus: bool,
    insertion_point: InsertionPoint,
    preview_images: HashMap<NodeId, egui::TextureId>,
}

impl App {
    fn new(cc: &eframe::CreationContext<'_>) -> Self {
        let resource_dir = directories::ProjectDirs::from("", "", "Radiance")
            .unwrap()
            .data_local_dir()
            .to_owned();

        if !resource_dir.exists() {
            fs::create_dir_all(&resource_dir).expect("Failed to create resource directory");
        }

        println!("Resource directory is: {}", resource_dir.display());

        load_default_library(&resource_dir);

        let egui_wgpu::RenderState { device, queue, .. } = cc.wgpu_render_state.as_ref().unwrap();
        let pixels_per_point = cc.egui_ctx.pixels_per_point();

        // Style

        cc.egui_ctx.set_theme(egui::Theme::Dark);
        cc.egui_ctx.style_mut(|style| {
            style.interaction.selectable_labels = false;
            style.visuals.handle_shape = egui::style::HandleShape::Circle;
        });

        // RADIANCE, WOO

        // Make a Mir
        let mir = Mir::new();

        // Make context
        let ctx = Context::new(resource_dir.clone(), device, queue);

        // Make widgets
        let waveform_widget = WaveformWidget::new(device, pixels_per_point);
        let spectrum_widget = SpectrumWidget::new(device, pixels_per_point);
        let beat_widget = BeatWidget::new(&device, pixels_per_point);

        let read_autosave_file = || {
            let contents = read_to_string(resource_dir.join(AUTOSAVE_FILENAME))
                .map_err(|e| format!("{:?}", e))?;
            serde_json::from_str(contents.as_str()).map_err(|e| format!("{:?}", e))
        };

        let props = read_autosave_file().unwrap_or_else(|err_string| {
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
                        "name": "logo.png",
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
        let render_target_list: HashMap<RenderTargetId, RenderTarget> =
            serde_json::from_value(json!({
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

        App {
            mir,
            ctx,
            props,
            render_target_list,
            auto_dj_1: None,
            auto_dj_2: None,
            auto_dj_1_enabled: false,
            auto_dj_2_enabled: false,
            autosave_timer: 0,
            preview_render_target_id,
            waveform_widget,
            spectrum_widget,
            beat_widget,
            waveform_texture: None,
            spectrum_texture: None,
            beat_texture: None,
            node_add_textedit: String::new(),
            left_panel_expanded: false,
            node_add_wants_focus: false,
            insertion_point: Default::default(),
            preview_images: Default::default(),
        }
    }
}

impl eframe::App for App {
    fn clear_color(&self, _visuals: &egui::Visuals) -> [f32; 4] {
        [0.2, 0.2, 0.2, 1.0]
    }

    fn update(&mut self, egui_ctx: &egui::Context, frame: &mut eframe::Frame) {
        let egui_wgpu::RenderState {
            device,
            queue,
            renderer,
            ..
        } = frame.wgpu_render_state.as_ref().unwrap();

        // Update
        let music_info = self.mir.poll();
        self.props.time = music_info.time;
        self.props.dt = music_info.tempo * (1. / 60.);
        self.props.audio = music_info.audio.clone();
        // Merge our render list and the winit_output render list into one:
        let render_target_list = self
            .render_target_list
            .iter()
            //.chain(winit_output.render_targets_iter()) // TODO
            .map(|(k, v)| (*k, v.clone()))
            .collect();
        //winit_output.update(event_loop, &mut props); // TODO
        self.auto_dj_1.as_mut().map(|a| {
            a.update(&mut self.props);

            // Uncheck the checkbox if we broke the AutoDJ
            if a.is_broken() {
                self.auto_dj_1_enabled = false;
            }
        });
        self.auto_dj_2.as_mut().map(|a| {
            a.update(&mut self.props);

            // Uncheck the checkbox if we broke the AutoDJ
            if a.is_broken() {
                self.auto_dj_2_enabled = false;
            }
        });

        self.ctx
            .update(device, queue, &mut self.props, &render_target_list);

        // Autosave if necessary
        // TODO: consider moving this to a background thread
        if self.autosave_timer == 0 {
            autosave(&self.ctx.resource_dir, &self.props);
            self.autosave_timer = AUTOSAVE_INTERVAL_FRAMES;
        } else {
            self.autosave_timer -= 1;
        }

        // Paint
        let results = {
            let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Encoder"),
            });

            let results =
                self.ctx
                    .paint(device, queue, &mut encoder, self.preview_render_target_id);

            queue.submit(iter::once(encoder.finish()));
            results
        };

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
        let spectrum_size = egui::vec2(330., 65.);
        let beat_size = egui::vec2(65., 65.);
        {
            let mut r = renderer.write();

            for node_id in self.props.graph.nodes.iter() {
                let native_texture = &results.get(&node_id).unwrap().view;
                match self.preview_images.entry(*node_id) {
                    Entry::Vacant(e) => {
                        e.insert(r.register_native_texture(
                            device,
                            native_texture,
                            wgpu::FilterMode::Linear,
                        ));
                    }
                    Entry::Occupied(e) => {
                        r.update_egui_texture_from_wgpu_texture(
                            device,
                            native_texture,
                            wgpu::FilterMode::Linear,
                            *e.get(),
                        );
                    }
                }
            }

            for (node_id, egui_texture_id) in self.preview_images.iter() {
                if !self.props.graph.nodes.contains(node_id) {
                    r.free_texture(egui_texture_id);
                }
            }

            // Update & paint widgets

            let waveform_native_texture = self.waveform_widget.paint(
                &device,
                &queue,
                waveform_size,
                &music_info.audio,
                music_info.uncompensated_unscaled_time,
            );

            update_or_register_native_texture(
                &mut r,
                &device,
                &waveform_native_texture.view,
                &mut self.waveform_texture,
            );

            let spectrum_native_texture =
                self.spectrum_widget
                    .paint(&device, &queue, spectrum_size, &music_info.spectrum);

            update_or_register_native_texture(
                &mut r,
                &device,
                &spectrum_native_texture.view,
                &mut self.spectrum_texture,
            );

            let beat_native_texture =
                self.beat_widget
                    .paint(device, queue, beat_size, music_info.unscaled_time);

            update_or_register_native_texture(
                &mut r,
                device,
                &beat_native_texture.view,
                &mut self.beat_texture,
            );
        }

        // EGUI update
        let left_panel_response =
            egui::SidePanel::left("left").show_animated(egui_ctx, self.left_panel_expanded, |ui| {
                ui.text_edit_singleline(&mut self.node_add_textedit)
            });

        let full_rect = egui_ctx.available_rect();
        egui::CentralPanel::default().show(egui_ctx, |ui| {
            let modal_id = ui.make_persistent_id("modal");
            let modal_shown = modal_shown(egui_ctx, modal_id);

            ui.scope_builder(
                {
                    let mut builder = egui::UiBuilder::default().max_rect(full_rect);
                    builder.disabled = modal_shown;
                    builder
                },
                |ui| {
                    ui.horizontal(|ui| {
                        ui.image((self.waveform_texture.unwrap(), waveform_size));
                        ui.image((self.spectrum_texture.unwrap(), spectrum_size));
                        ui.image((self.beat_texture.unwrap(), beat_size));
                        ui.checkbox(&mut self.auto_dj_1_enabled, "Auto DJ 1");
                        ui.checkbox(&mut self.auto_dj_2_enabled, "Auto DJ 2");

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
                        egui::ComboBox::from_id_salt("global timescale")
                            .selected_text(str_for_timescale(self.mir.global_timescale).as_str())
                            .show_ui(ui, |ui| {
                                for &timescale in timescales.iter() {
                                    ui.selectable_value(
                                        &mut self.mir.global_timescale,
                                        timescale,
                                        str_for_timescale(timescale).as_str(),
                                    );
                                }
                            });
                        ui.label("Latency compensation:");
                        ui.add(
                            egui::DragValue::new(&mut self.mir.latency_compensation)
                                .speed(0.001)
                                .fixed_decimals(3)
                                .suffix("s")
                                .range(0. ..=1.),
                        );
                    });

                    let mosaic_response = ui.add(mosaic(
                        "mosaic",
                        &mut self.props,
                        self.ctx.node_states(),
                        &self.preview_images,
                        &mut self.insertion_point,
                        modal_id,
                    ));

                    if !self.left_panel_expanded && ui.input(|i| i.key_pressed(egui::Key::A)) {
                        self.left_panel_expanded = true;
                        self.node_add_wants_focus = true;
                    }

                    if let Some(egui::InnerResponse {
                        inner: node_add_response,
                        response: _,
                    }) = left_panel_response
                    {
                        // TODO all this side-panel handling is wonky. It is done, in part, to avoid mutating the props before it's drawn.
                        // This needs to be factored out into a real "library" component.
                        if self.node_add_wants_focus {
                            node_add_response.request_focus();
                            self.node_add_wants_focus = false;
                        }
                        if node_add_response.lost_focus() {
                            if egui_ctx.input(|i| i.key_pressed(egui::Key::Enter)) {
                                let node_add_textedit_str = self.node_add_textedit.as_str();
                                if node_add_textedit_str.starts_with("http:")
                                    || node_add_textedit_str.starts_with("https:")
                                    || node_add_textedit_str.ends_with(".mp4")
                                    || node_add_textedit_str.ends_with(".mkv")
                                    || node_add_textedit_str.ends_with(".avi")
                                {
                                    #[cfg(feature = "mpv")]
                                    {
                                        let new_node_id = NodeId::gen();
                                        let new_node_props = NodeProps::MovieNode(MovieNodeProps {
                                            name: self.node_add_textedit.clone(),
                                            ..Default::default()
                                        });
                                        self.props.node_props.insert(new_node_id, new_node_props);
                                        self.props
                                            .graph
                                            .insert_node(new_node_id, &self.insertion_point);
                                    }
                                    #[cfg(not(feature = "mpv"))]
                                    {
                                        println!("Cannot instantiate MovieNode for {}, mpv support is not enabled", node_add_textedit_str)
                                    }
                                } else if node_add_textedit_str.ends_with(".png")
                                    || node_add_textedit_str.starts_with(".jpg")
                                    || node_add_textedit_str.ends_with(".gif")
                                {
                                    let new_node_id = NodeId::gen();
                                    let new_node_props = NodeProps::ImageNode(ImageNodeProps {
                                        name: self.node_add_textedit.clone(),
                                        ..Default::default()
                                    });
                                    self.props.node_props.insert(new_node_id, new_node_props);
                                    self.props
                                        .graph
                                        .insert_node(new_node_id, &self.insertion_point);
                                } else {
                                    match self.node_add_textedit.as_str() {
                                        "ScreenOutput" => {
                                            let new_node_id = NodeId::gen();
                                            let new_node_props = NodeProps::ScreenOutputNode(
                                                ScreenOutputNodeProps {
                                                    ..Default::default()
                                                },
                                            );
                                            self.props
                                                .node_props
                                                .insert(new_node_id, new_node_props);
                                            self.props
                                                .graph
                                                .insert_node(new_node_id, &self.insertion_point);
                                        }
                                        "ProjectionMappedOutput" => {
                                            let new_node_id = NodeId::gen();
                                            let new_node_props =
                                                NodeProps::ProjectionMappedOutputNode(
                                                    ProjectionMappedOutputNodeProps {
                                                        ..Default::default()
                                                    },
                                                );
                                            self.props
                                                .node_props
                                                .insert(new_node_id, new_node_props);
                                            self.props
                                                .graph
                                                .insert_node(new_node_id, &self.insertion_point);
                                        }
                                        _ => {
                                            let new_node_id = NodeId::gen();
                                            let new_node_props =
                                                NodeProps::EffectNode(EffectNodeProps {
                                                    name: self.node_add_textedit.clone(),
                                                    ..Default::default()
                                                });
                                            self.props
                                                .node_props
                                                .insert(new_node_id, new_node_props);
                                            self.props
                                                .graph
                                                .insert_node(new_node_id, &self.insertion_point);
                                            // TODO: select and focus the new node
                                            // (consider making selection & focus part of the explicit state of mosaic, not memory)
                                        }
                                    }
                                }
                            }
                            self.node_add_textedit.clear();
                            self.left_panel_expanded = false;
                            mosaic_response.request_focus();
                        }
                    }
                },
            );

            if modal_shown {
                ui.scope_builder(egui::UiBuilder::default().max_rect(full_rect), |ui| {
                    ui.add(modal(
                        modal_id,
                        &mut self.props,
                        self.ctx.node_states(),
                        &self.preview_images,
                    ));
                });
            }
        });

        // Construct or destroy the AutoDJs
        match (self.auto_dj_1_enabled, &mut self.auto_dj_1) {
            (false, Some(_)) => {
                self.auto_dj_1 = None;
            }
            (true, None) => {
                self.auto_dj_1 = Some(AutoDJ::new());
            }
            _ => {}
        }
        match (self.auto_dj_2_enabled, &mut self.auto_dj_2) {
            (false, Some(_)) => {
                self.auto_dj_2 = None;
            }
            (true, None) => {
                self.auto_dj_2 = Some(AutoDJ::new());
            }
            _ => {}
        }

        egui_ctx.request_repaint();
    }
}
