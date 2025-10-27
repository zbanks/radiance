#![cfg(any())] // TODO

/// This module handles radiance output through winit
/// (e.g. actually displaying ScreenOutputNode to a screen)
use egui_winit::winit;
use egui_winit::winit::{
    event::*,
    event_loop::EventLoopWindowTarget,
    window::{Fullscreen, WindowBuilder},
};
use nalgebra::Vector2;
use serde_json::json;
use std::collections::HashMap;
use std::iter;
use std::sync::Arc;

const COMMON_RESOLUTIONS: &[[u32; 2]] = &[
    [4096, 2160],
    [3840, 2160],
    [2560, 2048],
    [3440, 1440],
    [2560, 1600],
    [2560, 1440],
    [2048, 1536],
    [2560, 1080],
    [1920, 1200],
    [2048, 1080],
    [1920, 1080],
    [1600, 1200],
    [1680, 1050],
    [1680, 1050],
    [1440, 1080],
    [1400, 1050],
    [1600, 900],
    [1440, 960],
    [1280, 1024],
    [1440, 900],
    [1280, 960],
    [1280, 854],
    [1366, 768],
    [1280, 800],
    [1152, 864],
    [1280, 768],
    [1280, 720],
    [1152, 768],
    [1024, 768],
    [1024, 600],
    [1024, 576],
    [800, 600],
    [768, 576],
    [854, 480],
    [800, 480],
    [640, 480],
    [480, 320],
    [384, 288],
    [352, 288],
    [320, 240],
    [320, 200],
];

const ALLOW_SQUISH: f32 = 1.1;

const EPSILON: f32 = 0.0001;

fn cross(a: Vector2<f32>, b: Vector2<f32>) -> f32 {
    a[0] * b[1] - a[1] * b[0]
}

#[derive(Debug)]
pub struct WinitOutput<'a> {
    screen_output_shader_module: wgpu::ShaderModule,
    screen_output_bind_group_layout: wgpu::BindGroupLayout,
    screen_output_render_pipeline_layout: wgpu::PipelineLayout,

    projection_mapped_output_shader_module: wgpu::ShaderModule,
    projection_mapped_output_bind_group_layout: wgpu::BindGroupLayout,
    projection_mapped_output_render_pipeline_layout: wgpu::PipelineLayout,
    projection_mapped_output_vertex_buffer_layout: wgpu::VertexBufferLayout<'a>,

    screen_outputs: HashMap<radiance::NodeId, Option<VisibleScreenOutput>>,
    projection_mapped_outputs: HashMap<radiance::NodeId, Option<VisibleProjectionMappedOutput>>,
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
    request_close: bool,  // If set to True, delete this window on the next update
    name: String,
    resolution: [u32; 2],
}

#[derive(Debug)]
struct VisibleProjectionMappedSingleOutput {
    window: egui_winit::winit::window::Window,
    surface: wgpu::Surface,
    config: wgpu::SurfaceConfiguration,
    render_pipeline: wgpu::RenderPipeline,
    uniform_buffer: wgpu::Buffer,
    uniforms: ProjectionMapUniforms,
    vertex_buffer: wgpu::Buffer,
    index_buffer: wgpu::Buffer,
    vertices: Vec<ProjectionMapVertex>,
    indices: Vec<u16>,
    indices_count: u32,
}

#[derive(Debug)]
struct VisibleProjectionMappedOutput {
    // Resources
    screens: HashMap<String, VisibleProjectionMappedSingleOutput>,
    render_target_id: radiance::RenderTargetId,
    render_target: radiance::RenderTarget,
    resolution: [u32; 2],
    initial_update: bool, // Initialized to false, set to true on first update.
    request_close: bool,  // If set to True, delete this window on the next update
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

impl VisibleProjectionMappedSingleOutput {
    fn resize(&mut self, device: &wgpu::Device, new_size: winit::dpi::PhysicalSize<u32>) {
        if new_size.width > 0 && new_size.height > 0 {
            self.config.width = new_size.width;
            self.config.height = new_size.height;
            self.surface.configure(device, &self.config);
        }
    }
}

// The uniform buffer associated with the projection mapped shader
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct ProjectionMapUniforms {
    inv_map: [f32; 12],
}

// The vertex data associated with the projection mapped shader
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct ProjectionMapVertex {
    uv: [f32; 2],
}

// Max vertices in a single crop polygon
const MAX_VERTICES: usize = 128;
const MAX_INDICES: usize = 128;

impl WinitOutput<'_> {
    pub fn new(
        instance: Arc<wgpu::Instance>,
        adapter: Arc<wgpu::Adapter>,
        device: Arc<wgpu::Device>,
        queue: Arc<wgpu::Queue>,
    ) -> Self {
        let screen_output_shader_module =
            device.create_shader_module(wgpu::ShaderModuleDescriptor {
                label: Some("Screen output shader"),
                source: wgpu::ShaderSource::Wgsl(include_str!("output.wgsl").into()),
            });

        let projection_mapped_output_shader_module =
            device.create_shader_module(wgpu::ShaderModuleDescriptor {
                label: Some("Projection mapped output shader"),
                source: wgpu::ShaderSource::Wgsl(include_str!("projection_map.wgsl").into()),
            });

        let screen_output_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
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
                label: Some("screen output texture bind group layout"),
            });

        let projection_mapped_output_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
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
                    wgpu::BindGroupLayoutEntry {
                        binding: 2, // ProjectionMapUniforms
                        visibility: wgpu::ShaderStages::VERTEX,
                        ty: wgpu::BindingType::Buffer {
                            ty: wgpu::BufferBindingType::Uniform,
                            has_dynamic_offset: false,
                            min_binding_size: None,
                        },
                        count: None,
                    },
                ],
                label: Some("screen output texture bind group layout"),
            });

        let screen_output_render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Screen output Render Pipeline Layout"),
                bind_group_layouts: &[&screen_output_bind_group_layout],
                push_constant_ranges: &[],
            });

        let projection_mapped_output_render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Projection mapped output Render Pipeline Layout"),
                bind_group_layouts: &[&projection_mapped_output_bind_group_layout],
                push_constant_ranges: &[],
            });

        let projection_mapped_output_vertex_buffer_layout = wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<ProjectionMapVertex>() as wgpu::BufferAddress,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &[wgpu::VertexAttribute {
                offset: 0,
                shader_location: 0,
                format: wgpu::VertexFormat::Float32x2,
            }],
        };

        WinitOutput {
            instance,
            adapter,
            device,
            queue,

            screen_output_shader_module,
            screen_output_bind_group_layout,
            screen_output_render_pipeline_layout,

            projection_mapped_output_shader_module,
            projection_mapped_output_bind_group_layout,
            projection_mapped_output_render_pipeline_layout,
            projection_mapped_output_vertex_buffer_layout,

            screen_outputs: HashMap::<radiance::NodeId, Option<VisibleScreenOutput>>::new(),
            projection_mapped_outputs: HashMap::<
                radiance::NodeId,
                Option<VisibleProjectionMappedOutput>,
            >::new(),
        }
    }

    pub fn render_targets_iter(
        &self,
    ) -> impl Iterator<Item = (&radiance::RenderTargetId, &radiance::RenderTarget)> {
        self.screen_outputs
            .values()
            .flatten()
            .map(|screen_output| {
                (
                    &screen_output.render_target_id,
                    &screen_output.render_target,
                )
            })
            .chain(self.projection_mapped_outputs.values().flatten().map(
                |projection_mapped_output| {
                    (
                        &projection_mapped_output.render_target_id,
                        &projection_mapped_output.render_target,
                    )
                },
            ))
    }

    pub fn update<T>(
        &mut self,
        event_loop: &EventLoopWindowTarget<T>,
        props: &mut radiance::Props,
    ) {
        // Mark all nodes that we know about as having received their initial update.
        // Painting is gated on this being true,
        // because otherwise, we might try to paint a render target that the radiance context doesn't know about.
        // After the initial update, the radiance context is guaranteed to know about this screen output's render target.
        for screen_output in self.screen_outputs.values_mut().flatten() {
            screen_output.initial_update = true;
        }
        for projection_mapped_output in self.projection_mapped_outputs.values_mut().flatten() {
            projection_mapped_output.initial_update = true;
        }

        // Prune screen_outputs and projection_mapped_outputs of any nodes that are no longer present in the given graph
        self.screen_outputs.retain(|id, _| {
            props
                .node_props
                .get(id)
                .map(|node_props| matches!(node_props, radiance::NodeProps::ScreenOutputNode(_)))
                .unwrap_or(false)
        });
        self.projection_mapped_outputs.retain(|id, _| {
            props
                .node_props
                .get(id)
                .map(|node_props| {
                    matches!(
                        node_props,
                        radiance::NodeProps::ProjectionMappedOutputNode(_)
                    )
                })
                .unwrap_or(false)
        });

        // Construct screen_outputs and projection_mapped_outputs for any ScreenOutputNodes we didn't know about
        for (node_id, node_props) in props.node_props.iter() {
            match node_props {
                radiance::NodeProps::ScreenOutputNode(_) => {
                    if !self.screen_outputs.contains_key(node_id) {
                        self.screen_outputs.insert(*node_id, None);
                    }
                }
                radiance::NodeProps::ProjectionMappedOutputNode(_) => {
                    if !self.projection_mapped_outputs.contains_key(node_id) {
                        self.projection_mapped_outputs.insert(*node_id, None);
                    }
                }
                _ => {}
            }
        }

        // See what screens are available for output
        let available_screens: Vec<radiance::AvailableOutputScreen> = event_loop
            .available_monitors()
            .filter_map(|mh| {
                let name = mh.name()?;
                let native_resolution = mh.size();
                if native_resolution.width * native_resolution.height == 0 {
                    return None;
                }
                let suggested_resolutions =
                    Some([native_resolution.width, native_resolution.height]).into_iter();
                let aspect = native_resolution.width as f32 / native_resolution.height as f32;
                let suggested_resolutions = suggested_resolutions.chain(
                    COMMON_RESOLUTIONS
                        .iter()
                        .filter(|resolution| {
                            if resolution[0] >= native_resolution.width
                                || resolution[1] >= native_resolution.height
                            {
                                return false; // Don't recommend higher resolutions than native resolution
                            }
                            let test_aspect = resolution[0] as f32 / resolution[1] as f32;
                            aspect / test_aspect < ALLOW_SQUISH
                                && test_aspect / aspect < ALLOW_SQUISH
                        })
                        .cloned(),
                );
                let suggested_resolutions: Vec<[u32; 2]> = suggested_resolutions.collect();
                if suggested_resolutions.is_empty() {
                    return None;
                }
                Some(radiance::AvailableOutputScreen {
                    name,
                    suggested_resolutions,
                })
            })
            .collect();

        // Update internal state of screen_outputs from props
        let node_ids: Vec<radiance::NodeId> = self.screen_outputs.keys().cloned().collect();
        for node_id in node_ids {
            let screen_output_props: &mut radiance::ScreenOutputNodeProps = props
                .node_props
                .get_mut(&node_id)
                .unwrap()
                .try_into()
                .unwrap();

            // If this node's screen list is totally empty, default it to the first screen
            if screen_output_props.available_screens.is_empty()
                && screen_output_props.screen.is_none()
                && available_screens
                    .first()
                    .map(|s| s.suggested_resolutions.first())
                    .is_some()
            {
                let screen = available_screens.first().unwrap();
                let name = screen.name.clone();
                let resolution = *screen.suggested_resolutions.first().unwrap();
                screen_output_props.screen =
                    Some(radiance::SelectedOutputScreen { name, resolution });
            }

            // Populate each screen output node props with a list of screens available on the system
            screen_output_props.available_screens = available_screens.clone();
            #[allow(clippy::if_same_then_else)]
            if screen_output_props.screen.is_none() {
                // Can't output if there's no screen selected
                screen_output_props.visible = false;
            } else if !available_screens.iter().any(|available_screen| {
                available_screen.name == screen_output_props.screen.as_ref().unwrap().name
            }) {
                // Hide any outputs that point to screens we don't know about
                screen_output_props.visible = false;
            }
            if self
                .screen_outputs
                .get(&node_id)
                .unwrap()
                .as_ref()
                .map(|visible_screen_output| visible_screen_output.request_close)
                .unwrap_or(false)
            {
                // Close was requested; set visible to false
                screen_output_props.visible = false;
            }

            if screen_output_props.visible {
                let screen = screen_output_props.screen.as_ref().unwrap(); // The selected screen, as per the props
                                                                           // (it is safe to unwrap it because the window would have been set to invisible had it been None)
                let needs_new_window = match self.screen_outputs.get(&node_id).unwrap() {
                    None => true, // Make a new window if we weren't visible last frame
                    Some(screen_output) => screen_output.resolution != screen.resolution, // Make a new window if the resolution changes TODO also make a new window if screen name changes
                };
                if needs_new_window {
                    let visible_screen_output =
                        self.new_screen_output(event_loop, &screen.name, &screen.resolution);
                    visible_screen_output.window.set_title("Radiance Output");
                    // Replace None with Some
                    self.screen_outputs
                        .insert(node_id, Some(visible_screen_output));
                }
            } else if self.screen_outputs.get(&node_id).unwrap().is_some() {
                // Replace Some with None if we aren't supposed to be visible
                self.screen_outputs.insert(node_id, None);
            }
        }

        // Update internal state of projection_mapped_outputs from props
        let node_ids: Vec<radiance::NodeId> =
            self.projection_mapped_outputs.keys().cloned().collect();
        for node_id in node_ids {
            let projection_mapped_output_props: &mut radiance::ProjectionMappedOutputNodeProps =
                props
                    .node_props
                    .get_mut(&node_id)
                    .unwrap()
                    .try_into()
                    .unwrap();

            // Populate each projection mapped output node props with a list of screens available on the system
            projection_mapped_output_props.available_screens = available_screens.clone();

            if self
                .projection_mapped_outputs
                .get(&node_id)
                .unwrap()
                .as_ref()
                .map(|visible_projection_mapped_output| {
                    visible_projection_mapped_output.request_close
                })
                .unwrap_or(false)
            {
                // Close was requested; set visible to false
                projection_mapped_output_props.visible = false;
            }

            if projection_mapped_output_props.visible {
                let needs_new_windows = match self.projection_mapped_outputs.get(&node_id).unwrap()
                {
                    None => true, // Make new resources if we weren't visible last frame
                    Some(projection_mapped_output) => {
                        projection_mapped_output.resolution
                            != projection_mapped_output_props.resolution
                    } // Make new resources if the resolution changes OR (TODO) screen list changes
                };
                if needs_new_windows {
                    let render_target_id = radiance::RenderTargetId::gen();
                    let render_target: radiance::RenderTarget = serde_json::from_value(json!({
                        "width": projection_mapped_output_props.resolution[0],
                        "height": projection_mapped_output_props.resolution[1],
                        "dt": 1. / 60.
                    }))
                    .unwrap();
                    let visible_projection_mapped_output = VisibleProjectionMappedOutput {
                        screens: projection_mapped_output_props
                            .screens
                            .iter()
                            .map(|single_screen_props| {
                                let single_output =
                                    self.new_projection_mapped_single_output(event_loop);
                                single_output.window.set_title("Radiance Output");
                                (single_screen_props.name.clone(), single_output)
                            })
                            .collect(),
                        render_target_id,
                        render_target,
                        initial_update: false,
                        request_close: false,
                        resolution: projection_mapped_output_props.resolution,
                    };
                    self.projection_mapped_outputs
                        .insert(node_id, Some(visible_projection_mapped_output));
                }

                let output = self
                    .projection_mapped_outputs
                    .get_mut(&node_id)
                    .unwrap()
                    .as_mut()
                    .unwrap();
                for screen_props in projection_mapped_output_props.screens.iter() {
                    if let Some(single_output) = output.screens.get_mut(&screen_props.name) {
                        let inv_map = screen_props.map.try_inverse().unwrap();
                        let inv_map_padded = [
                            inv_map[(0, 0)],
                            inv_map[(1, 0)],
                            inv_map[(2, 0)],
                            0.,
                            inv_map[(0, 1)],
                            inv_map[(1, 1)],
                            inv_map[(2, 1)],
                            0.,
                            inv_map[(0, 2)],
                            inv_map[(1, 2)],
                            inv_map[(2, 2)],
                            0.,
                        ];
                        single_output.uniforms = ProjectionMapUniforms {
                            inv_map: inv_map_padded,
                        };

                        single_output.vertices = screen_props
                            .crop
                            .iter()
                            .map(|uv| ProjectionMapVertex { uv: [uv[0], uv[1]] })
                            .collect();
                        // Ear-clip polygon from props to get the index list

                        let mut indices: Vec<usize> = (0..screen_props.crop.len()).collect();
                        single_output.indices.clear();
                        while indices.len() > 2 {
                            // Find an ear
                            let mut found_ear = false;
                            for i in 0..indices.len() {
                                let ii1 = (indices.len() + i - 1) % indices.len();
                                let ii2 = i;
                                let ii3 = (i + 1) % indices.len();
                                let p1 = screen_props.crop[indices[ii1]];
                                let p2 = screen_props.crop[indices[ii2]];
                                let p3 = screen_props.crop[indices[ii3]];
                                let c = cross(p2 - p1, p3 - p2);
                                // CCW polygon winding produces negative cross products on interior edges
                                if c < -EPSILON {
                                    // This vertex is an ear tip if there all vertices are outside the ear
                                    let edge_inside_polygon = indices.iter().all(|&test_ix| {
                                        let test_vertex = screen_props.crop[test_ix];

                                        // Inside triangle test, from http://blackpawn.com/texts/pointinpoly/
                                        let v0 = p3 - p1;
                                        let v1 = p2 - p1;
                                        let v2 = test_vertex - p1;

                                        // Compute dot products
                                        let dot00 = v0.dot(&v0);
                                        let dot01 = v0.dot(&v1);
                                        let dot02 = v0.dot(&v2);
                                        let dot11 = v1.dot(&v1);
                                        let dot12 = v1.dot(&v2);

                                        // Compute barycentric coordinates
                                        let inv_denom = 1. / (dot00 * dot11 - dot01 * dot01);
                                        let u = (dot11 * dot02 - dot01 * dot12) * inv_denom;
                                        let v = (dot00 * dot12 - dot01 * dot02) * inv_denom;

                                        // Check if point is outside triangle
                                        !((u >= EPSILON)
                                            && (v >= EPSILON)
                                            && (u + v < 1. - EPSILON))
                                    });
                                    if edge_inside_polygon {
                                        found_ear = true;
                                        single_output.indices.push(indices[ii1] as u16);
                                        single_output.indices.push(indices[ii2] as u16);
                                        single_output.indices.push(indices[ii3] as u16);
                                        indices.remove(i); // Clip
                                        break;
                                    }
                                }
                            }
                            if !found_ear {
                                // We have clipped the polygon to the point that it has no ears
                                break;
                            }
                        }
                        single_output.indices_count = single_output.indices.len() as u32;
                        if single_output.indices_count % 2 == 1 {
                            single_output.indices.push(0); // Add padding to get to a multiple of 4 bytes
                        }
                    }
                }
            } else if self
                .projection_mapped_outputs
                .get(&node_id)
                .unwrap()
                .is_some()
            {
                // Replace Some with None if we aren't supposed to be visible
                self.projection_mapped_outputs.insert(node_id, None);
            }
        }
    }

    fn new_screen_output<T>(
        &self,
        event_loop: &EventLoopWindowTarget<T>,
        name: &str,
        resolution: &[u32; 2],
    ) -> VisibleScreenOutput {
        let window = WindowBuilder::new().build(event_loop).unwrap();
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

        let render_pipeline = self
            .device
            .create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("Output Render Pipeline"),
                layout: Some(&self.screen_output_render_pipeline_layout),
                vertex: wgpu::VertexState {
                    module: &self.screen_output_shader_module,
                    entry_point: Some("vs_main"),
                    buffers: &[],
                    compilation_options: Default::default(),
                },
                fragment: Some(wgpu::FragmentState {
                    module: &self.screen_output_shader_module,
                    entry_point: Some("fs_main"),
                    targets: &[Some(wgpu::ColorTargetState {
                        format: config.format,
                        blend: Some(wgpu::BlendState::REPLACE),
                        write_mask: wgpu::ColorWrites::ALL,
                    })],
                    compilation_options: Default::default(),
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
                cache: None,
            });

        let render_target_id = radiance::RenderTargetId::gen();
        let render_target: radiance::RenderTarget = serde_json::from_value(json!({
            "width": resolution[0],
            "height": resolution[1],
            "dt": 1. / 60.
        }))
        .unwrap();

        VisibleScreenOutput {
            window,
            surface,
            config,
            render_pipeline,
            render_target_id,
            render_target,
            initial_update: false,
            request_close: false,
            name: name.to_owned(),
            resolution: *resolution,
        }
    }

    fn new_projection_mapped_single_output<T>(
        &self,
        event_loop: &EventLoopWindowTarget<T>,
    ) -> VisibleProjectionMappedSingleOutput {
        let window = WindowBuilder::new().build(event_loop).unwrap();
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

        let render_pipeline = self
            .device
            .create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("Output Render Pipeline"),
                layout: Some(&self.projection_mapped_output_render_pipeline_layout),
                vertex: wgpu::VertexState {
                    module: &self.projection_mapped_output_shader_module,
                    entry_point: Some("vs_main"),
                    buffers: &[self.projection_mapped_output_vertex_buffer_layout.clone()],
                    compilation_options: Default::default(),
                },
                fragment: Some(wgpu::FragmentState {
                    module: &self.projection_mapped_output_shader_module,
                    entry_point: Some("fs_main"),
                    targets: &[Some(wgpu::ColorTargetState {
                        format: config.format,
                        blend: Some(wgpu::BlendState::REPLACE),
                        write_mask: wgpu::ColorWrites::ALL,
                    })],
                    compilation_options: Default::default(),
                }),
                primitive: wgpu::PrimitiveState {
                    topology: wgpu::PrimitiveTopology::TriangleList,
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
                cache: None,
            });

        let uniform_buffer = self.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("Projection mapped output uniform buffer"),
            size: std::mem::size_of::<ProjectionMapUniforms>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let vertex_buffer = self.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("Projection mapped output vertex buffer"),
            size: (std::mem::size_of::<ProjectionMapVertex>() * MAX_VERTICES) as u64,
            usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let index_buffer = self.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("Projection mapped output index buffer"),
            size: (std::mem::size_of::<u16>() * MAX_INDICES) as u64,
            usage: wgpu::BufferUsages::INDEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        VisibleProjectionMappedSingleOutput {
            window,
            surface,
            config,
            render_pipeline,
            uniform_buffer,
            uniforms: ProjectionMapUniforms { inv_map: [0.; 12] },
            vertex_buffer,
            index_buffer,
            vertices: Vec::<_>::new(),
            indices: Vec::<_>::new(),
            indices_count: 0,
        }
    }

    pub fn on_event<T>(
        &mut self,
        event: &Event<T>,
        event_loop: &EventLoopWindowTarget<T>,
        ctx: &mut radiance::Context,
    ) -> bool {
        // Return true => event consumed
        // Return false => event continues to be processed

        for (node_id, screen_output) in self
            .screen_outputs
            .iter_mut()
            .filter_map(|(k, v)| Some((k, v.as_mut()?)))
        {
            match event {
                Event::RedrawRequested(window_id) if window_id == &screen_output.window.id() => {
                    if screen_output.initial_update {
                        // Fullscreen
                        let mh = event_loop.available_monitors().find(|mh| {
                            mh.name()
                                .map(|n| &n == &screen_output.name)
                                .unwrap_or(false)
                        });
                        if mh.is_some() {
                            screen_output
                                .window
                                .set_fullscreen(Some(Fullscreen::Borderless(mh.clone())));
                        }

                        // Paint
                        let mut encoder =
                            self.device
                                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                                    label: Some("Output Encoder"),
                                });

                        let results = ctx.paint(&mut encoder, screen_output.render_target_id);

                        if let Some(texture) = results.get(node_id) {
                            let output_bind_group =
                                self.device.create_bind_group(&wgpu::BindGroupDescriptor {
                                    layout: &self.screen_output_bind_group_layout,
                                    entries: &[
                                        wgpu::BindGroupEntry {
                                            binding: 0,
                                            resource: wgpu::BindingResource::TextureView(
                                                &texture.view,
                                            ),
                                        },
                                        wgpu::BindGroupEntry {
                                            binding: 1,
                                            resource: wgpu::BindingResource::Sampler(
                                                &texture.sampler,
                                            ),
                                        },
                                    ],
                                    label: Some("output bind group"),
                                });

                            // Record output render pass.
                            let output = screen_output.surface.get_current_texture().unwrap();
                            let view = output
                                .texture
                                .create_view(&wgpu::TextureViewDescriptor::default());

                            {
                                let mut render_pass =
                                    encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                                        label: Some("Output window render pass"),
                                        color_attachments: &[Some(
                                            wgpu::RenderPassColorAttachment {
                                                view: &view,
                                                resolve_target: None,
                                                ops: wgpu::Operations {
                                                    load: wgpu::LoadOp::Clear(wgpu::Color {
                                                        r: 0.,
                                                        g: 0.,
                                                        b: 0.,
                                                        a: 0.,
                                                    }),
                                                    store: wgpu::StoreOp::Store,
                                                },
                                                depth_slice: None,
                                            },
                                        )],
                                        depth_stencil_attachment: None,
                                        timestamp_writes: None,
                                        occlusion_query_set: None,
                                    });

                                render_pass.set_pipeline(&screen_output.render_pipeline);
                                render_pass.set_bind_group(0, &output_bind_group, &[]);
                                render_pass.draw(0..6, 0..1);
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
        for (node_id, output) in self
            .projection_mapped_outputs
            .iter_mut()
            .filter_map(|(k, v)| Some((k, v.as_mut()?)))
        {
            for (screen_name, single_output) in output.screens.iter_mut() {
                match event {
                    Event::RedrawRequested(window_id)
                        if window_id == &single_output.window.id() =>
                    {
                        if output.initial_update {
                            // Fullscreen
                            let mh = event_loop
                                .available_monitors()
                                .find(|mh| mh.name().map(|n| &n == screen_name).unwrap_or(false));
                            if mh.is_some() {
                                single_output
                                    .window
                                    .set_fullscreen(Some(Fullscreen::Borderless(mh.clone())));
                            }

                            // Write uniforms
                            self.queue.write_buffer(
                                &single_output.uniform_buffer,
                                0,
                                bytemuck::cast_slice(&[single_output.uniforms]),
                            );

                            // Write vertices
                            self.queue.write_buffer(
                                &single_output.vertex_buffer,
                                0,
                                bytemuck::cast_slice(&single_output.vertices),
                            );
                            self.queue.write_buffer(
                                &single_output.index_buffer,
                                0,
                                bytemuck::cast_slice(&single_output.indices),
                            );

                            // Paint
                            let mut encoder = self.device.create_command_encoder(
                                &wgpu::CommandEncoderDescriptor {
                                    label: Some("Output Encoder"),
                                },
                            );

                            let results = ctx.paint(&mut encoder, output.render_target_id);

                            if let Some(texture) = results.get(node_id) {
                                let output_bind_group =
                                    self.device.create_bind_group(&wgpu::BindGroupDescriptor {
                                        layout: &self.projection_mapped_output_bind_group_layout,
                                        entries: &[
                                            wgpu::BindGroupEntry {
                                                binding: 0,
                                                resource: wgpu::BindingResource::TextureView(
                                                    &texture.view,
                                                ),
                                            },
                                            wgpu::BindGroupEntry {
                                                binding: 1,
                                                resource: wgpu::BindingResource::Sampler(
                                                    &texture.sampler,
                                                ),
                                            },
                                            wgpu::BindGroupEntry {
                                                binding: 2,
                                                resource: single_output
                                                    .uniform_buffer
                                                    .as_entire_binding(),
                                            },
                                        ],
                                        label: Some("output bind group"),
                                    });

                                // Record output render pass.
                                let output = single_output.surface.get_current_texture().unwrap();
                                let view = output
                                    .texture
                                    .create_view(&wgpu::TextureViewDescriptor::default());

                                {
                                    let mut render_pass =
                                        encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                                            label: Some("Output window render pass"),
                                            color_attachments: &[Some(
                                                wgpu::RenderPassColorAttachment {
                                                    view: &view,
                                                    resolve_target: None,
                                                    ops: wgpu::Operations {
                                                        load: wgpu::LoadOp::Clear(wgpu::Color {
                                                            r: 0.,
                                                            g: 0.,
                                                            b: 0.,
                                                            a: 0.,
                                                        }),
                                                        store: wgpu::StoreOp::Store,
                                                    },
                                                    depth_slice: None,
                                                },
                                            )],
                                            depth_stencil_attachment: None,
                                            timestamp_writes: None,
                                            occlusion_query_set: None,
                                        });

                                    render_pass.set_pipeline(&single_output.render_pipeline);
                                    render_pass.set_bind_group(0, &output_bind_group, &[]);
                                    render_pass.set_vertex_buffer(
                                        0,
                                        single_output.vertex_buffer.slice(..),
                                    );
                                    render_pass.set_index_buffer(
                                        single_output.index_buffer.slice(..),
                                        wgpu::IndexFormat::Uint16,
                                    );
                                    render_pass.draw_indexed(
                                        0..single_output.indices_count,
                                        0,
                                        0..1,
                                    );
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
                    } if window_id == &single_output.window.id() => {
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
                                output.request_close = true;
                            }
                            WindowEvent::Resized(physical_size) => {
                                let output_size = *physical_size;
                                single_output.resize(&self.device, output_size);
                            }
                            WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                                let output_size = **new_inner_size;
                                single_output.resize(&self.device, output_size);
                            }
                            _ => {}
                        }
                        return true;
                    }
                    Event::MainEventsCleared => {
                        single_output.window.request_redraw();
                    }
                    _ => {}
                }
            }
        }
        false
    }
}
