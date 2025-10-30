use crate::effect_node::EffectNodeState;
use crate::image_node::ImageNodeState;

#[cfg(feature = "mpv")]
use crate::movie_node::MovieNodeState;

use crate::placeholder_node::PlaceholderNodeState;
use crate::projection_mapped_output_node::ProjectionMappedOutputNodeState;
use crate::render_target::{RenderTarget, RenderTargetId};
use crate::screen_output_node::ScreenOutputNodeState;
use crate::{AudioLevels, Graph, NodeId, NodeProps, Props};
use rand::Rng;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::io;
use std::path::PathBuf;
use std::sync::Arc;

const MAX_TIME: f32 = 64.;

/// A bundle of a texture, a texture view, and a sampler.
/// Each is stored within an `Arc` for sharing between threads
/// if necessary.
#[derive(Clone, Debug)]
pub struct ArcTextureViewSampler {
    pub texture: Arc<wgpu::Texture>,
    pub view: Arc<wgpu::TextureView>,
    pub sampler: Arc<wgpu::Sampler>,
}

// TODO are all three of these really necessary? I think most are unused.

impl ArcTextureViewSampler {
    /// Bundle together a texture, a texture view, and a sampler
    pub fn new(texture: wgpu::Texture, view: wgpu::TextureView, sampler: wgpu::Sampler) -> Self {
        Self {
            texture: Arc::new(texture),
            view: Arc::new(view),
            sampler: Arc::new(sampler),
        }
    }
}

/// An enum describing how media fits to a screen with a different aspect ratio
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub enum Fit {
    Crop,
    Shrink,
    Zoom,
}

impl Fit {
    /// This zoom factor eliminates top / bottom black bars
    /// on videos that are recorded in 21:9 aspect
    /// and letterboxed to 16:9
    pub const ZOOM_FACTOR: f32 = 16. / 21.;

    /// Returns the necessary scale factor to paint media of the given size onto a canvas of the
    /// given size (using the given mode)
    pub fn factor(&self, media_size: (f32, f32), canvas_size: (f32, f32)) -> (f32, f32) {
        let (media_width, media_height) = media_size;
        let (canvas_width, canvas_height) = canvas_size;
        let factor_fit_x = media_height * canvas_width / media_width / canvas_height;
        let factor_fit_y = media_width * canvas_height / media_height / canvas_width;

        match &self {
            Self::Shrink => (factor_fit_x.max(1.), factor_fit_y.max(1.)),
            Self::Zoom => (factor_fit_x * Self::ZOOM_FACTOR, Self::ZOOM_FACTOR),
            Self::Crop => (factor_fit_x.min(1.), factor_fit_y.min(1.)),
        }
    }
}

/// A `Context` bundles all of the state and graphics resources
/// necessary to support iterated rendering of a `Props`.
///
/// `Props` and `RenderTargetList` are intentionally kept stateless
/// and untangled from system resources.
/// We push that complexity into this object.
/// The `Context` caches objects for reuse and preserves state
/// from frame to frame based on render target / node ID.
///
/// If given render targets or nodes that were not previously `paint`ed,
/// the `Context` will carve out new state for them
/// and remember it for next time.
/// If previously rendered targets or nodes are omitted
/// in a subsequent `paint` call,
/// the `Context` will drop their state.
pub struct Context {
    // OS resources
    pub resource_dir: PathBuf,

    // Graphics resources
    blank_texture: ArcTextureViewSampler,

    // Cached props from the last update()
    pub time: f32,
    pub dt: f32,
    pub audio: AudioLevels,
    graph: Graph,
    graph_input_mapping: HashMap<NodeId, Vec<Option<NodeId>>>,

    // State of individual render targets and nodes
    render_target_states: HashMap<RenderTargetId, RenderTargetState>,
    node_states: HashMap<NodeId, NodeState>,
}

/// Internal state and resources that is associated with a specific RenderTarget,
/// but not with any specific node.
pub struct RenderTargetState {
    width: u32,
    height: u32,
    dt: f32,
    noise_texture: ArcTextureViewSampler,
}

/// Internal state and resources that is associated with a specific Node
#[derive(derive_more::TryInto)]
#[try_into(owned, ref, ref_mut)]
#[allow(clippy::large_enum_variant)]
pub enum NodeState {
    EffectNode(EffectNodeState),
    ScreenOutputNode(ScreenOutputNodeState),
    ImageNode(ImageNodeState),
    PlaceholderNode(PlaceholderNodeState),
    #[cfg(feature = "mpv")]
    MovieNode(MovieNodeState),
    ProjectionMappedOutputNode(ProjectionMappedOutputNodeState),
}

impl Context {
    fn create_blank_texture(device: &wgpu::Device, queue: &wgpu::Queue) -> ArcTextureViewSampler {
        let texture_size = wgpu::Extent3d {
            width: 1,
            height: 1,
            depth_or_array_layers: 1,
        };
        let texture = device.create_texture(&wgpu::TextureDescriptor {
            size: texture_size,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            label: Some("blank texture"),
            view_formats: &[wgpu::TextureFormat::Rgba8Unorm],
        });

        queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &[0, 0, 0, 0],
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(4),
                rows_per_image: Some(1),
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Nearest,
            min_filter: wgpu::FilterMode::Nearest,
            mipmap_filter: wgpu::FilterMode::Nearest,
            ..Default::default()
        });

        ArcTextureViewSampler::new(texture, view, sampler)
    }

    fn create_noise_texture(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        width: u32,
        height: u32,
    ) -> ArcTextureViewSampler {
        let texture_size = wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        };
        let texture = device.create_texture(&wgpu::TextureDescriptor {
            size: texture_size,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            label: Some("noise texture"),
            view_formats: &[wgpu::TextureFormat::Rgba8Unorm],
        });

        let random_bytes: Vec<u8> = (0..width * height * 4)
            .map(|_| rand::thread_rng().gen::<u8>())
            .collect();

        queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &random_bytes,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(4 * width),
                rows_per_image: Some(height),
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::Repeat,
            address_mode_v: wgpu::AddressMode::Repeat,
            address_mode_w: wgpu::AddressMode::Repeat,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });

        ArcTextureViewSampler::new(texture, view, sampler)
    }

    /// Create a new context with the given graphics resources.
    /// dt is the expected period (in seconds) with which update() will be called.
    /// This is distinct from the period at which paint() may be called,
    /// and the period for that is set in the render target.
    pub fn new(resource_dir: PathBuf, device: &wgpu::Device, queue: &wgpu::Queue) -> Self {
        let blank_texture = Self::create_blank_texture(&device, &queue);
        Self {
            resource_dir,
            blank_texture,
            time: 0.,
            dt: 0.,
            audio: Default::default(),
            graph: Default::default(),
            graph_input_mapping: Default::default(),
            render_target_states: Default::default(),
            node_states: Default::default(),
        }
    }

    fn new_render_target_state(
        &self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        render_target: &RenderTarget,
    ) -> RenderTargetState {
        RenderTargetState {
            width: render_target.width(),
            height: render_target.height(),
            dt: render_target.dt(),
            noise_texture: Self::create_noise_texture(
                device,
                queue,
                render_target.width(),
                render_target.height(),
            ),
        }
    }

    fn new_node_state(
        &self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        node_props: &NodeProps,
    ) -> NodeState {
        match node_props {
            NodeProps::EffectNode(props) => {
                NodeState::EffectNode(EffectNodeState::new(self, device, queue, props))
            }
            NodeProps::ScreenOutputNode(props) => {
                NodeState::ScreenOutputNode(ScreenOutputNodeState::new(self, device, queue, props))
            }
            NodeProps::ImageNode(props) => {
                NodeState::ImageNode(ImageNodeState::new(self, device, queue, props))
            }
            NodeProps::PlaceholderNode(props) => {
                NodeState::PlaceholderNode(PlaceholderNodeState::new(self, device, queue, props))
            }
            #[cfg(feature = "mpv")]
            NodeProps::MovieNode(props) => {
                NodeState::MovieNode(MovieNodeState::new(self, device, queue, props))
            }
            NodeProps::ProjectionMappedOutputNode(props) => NodeState::ProjectionMappedOutputNode(
                ProjectionMappedOutputNodeState::new(self, device, queue, props),
            ),
        }
    }

    fn update_node(
        &mut self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        node_id: NodeId,
        node_props: &mut NodeProps,
    ) {
        let mut node_state = self.node_states.remove(&node_id).unwrap();
        match node_state {
            NodeState::EffectNode(ref mut state) => match node_props {
                NodeProps::EffectNode(ref mut props) => state.update(self, device, queue, props),
                _ => panic!("Type mismatch between props and state"),
            },
            NodeState::ScreenOutputNode(ref mut state) => match node_props {
                NodeProps::ScreenOutputNode(ref mut props) => {
                    state.update(self, device, queue, props)
                }
                _ => panic!("Type mismatch between props and state"),
            },
            NodeState::ImageNode(ref mut state) => match node_props {
                NodeProps::ImageNode(ref mut props) => state.update(self, device, queue, props),
                _ => panic!("Type mismatch between props and state"),
            },
            NodeState::PlaceholderNode(ref mut state) => match node_props {
                NodeProps::PlaceholderNode(ref mut props) => {
                    state.update(self, device, queue, props)
                }
                _ => panic!("Type mismatch between props and state"),
            },
            #[cfg(feature = "mpv")]
            NodeState::MovieNode(ref mut state) => match node_props {
                NodeProps::MovieNode(ref mut props) => state.update(self, device, queue, props),
                _ => panic!("Type mismatch between props and state"),
            },
            NodeState::ProjectionMappedOutputNode(ref mut state) => match node_props {
                NodeProps::ProjectionMappedOutputNode(ref mut props) => {
                    state.update(self, device, queue, props)
                }
                _ => panic!("Type mismatch between props and state"),
            },
        };
        self.node_states.insert(node_id, node_state);
    }

    fn paint_node(
        &mut self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        encoder: &mut wgpu::CommandEncoder,
        node_id: NodeId,
        render_target_id: RenderTargetId,
        input_textures: &[Option<ArcTextureViewSampler>],
    ) -> ArcTextureViewSampler {
        let mut node_state = self.node_states.remove(&node_id).unwrap();
        let result = match node_state {
            NodeState::EffectNode(ref mut state) => state.paint(
                self,
                device,
                queue,
                encoder,
                render_target_id,
                input_textures,
            ),
            NodeState::ScreenOutputNode(ref mut state) => state.paint(
                self,
                device,
                queue,
                encoder,
                render_target_id,
                input_textures,
            ),
            NodeState::ImageNode(ref mut state) => state.paint(
                self,
                device,
                queue,
                encoder,
                render_target_id,
                input_textures,
            ),
            NodeState::PlaceholderNode(ref mut state) => state.paint(
                self,
                device,
                queue,
                encoder,
                render_target_id,
                input_textures,
            ),
            #[cfg(feature = "mpv")]
            NodeState::MovieNode(ref mut state) => state.paint(
                self,
                device,
                queue,
                encoder,
                render_target_id,
                input_textures,
            ),
            NodeState::ProjectionMappedOutputNode(ref mut state) => state.paint(
                self,
                device,
                queue,
                encoder,
                render_target_id,
                input_textures,
            ),
        };
        self.node_states.insert(node_id, node_state);
        result
    }
    /// Update the internal state of `Context` to match the given `Props`.
    /// `update` tries to run quickly.
    /// If a newly added node needs time to load resources and initializing,
    /// the node will likely do this asynchronously.
    /// In the meantime, it will probably pass through its input unchanged or return a blank texture.
    /// The exception to this rule is when render targets are added;
    /// node-independent render target initialization (such as generating the noise texture)
    /// happens synchronously, so the `update` call may take a long time
    /// and rendering may stutter.
    ///
    /// The passed in Props will be mutated to advance its contents by one timestep.
    /// It can then be further mutated before calling update() again
    /// (or replaced entirely.)
    pub fn update(
        &mut self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        props: &mut Props,
        render_targets: &HashMap<RenderTargetId, RenderTarget>,
    ) {
        // 1. Store graph topology for rendering

        // Make sure the props are well-formed
        // (e.g. there is a .node_props entry for every node in the .graph)
        // and re-compute the graph topology if the graph changed
        props.fix();

        if self.graph != props.graph {
            props.graph.fix();
            let (_, input_mapping) = props.graph.mapping();
            self.graph_input_mapping = input_mapping;
            self.graph = props.graph.clone();
        }

        // 2. Sample-and-hold global props like `time` and `dt`, then update them
        self.time = props.time;
        self.dt = props.dt;
        self.audio = props.audio.clone();

        props.time = (props.time + props.dt).rem_euclid(MAX_TIME);

        // 3. Prune render_target_states and node_states of any nodes or render_targets that are no longer present in the given graph/render_targets

        self.render_target_states
            .retain(|id, _| render_targets.contains_key(id));
        self.node_states
            .retain(|id, _| self.graph_input_mapping.contains_key(id));

        // 4. Construct any missing render_target_states or node_states (this may kick of background processing)

        for (check_render_target_id, render_target) in render_targets.iter() {
            if !self
                .render_target_states
                .contains_key(check_render_target_id)
            {
                self.render_target_states.insert(
                    *check_render_target_id,
                    self.new_render_target_state(device, queue, render_target),
                );
            }
        }

        for check_node_id in props.graph.nodes.iter() {
            if !self.node_states.contains_key(check_node_id) {
                self.node_states.insert(
                    *check_node_id,
                    self.new_node_state(
                        device,
                        queue,
                        props.node_props.get(check_node_id).unwrap(),
                    ),
                );
            }
        }

        // 5. Update state on every node
        let nodes: Vec<NodeId> = props.graph.nodes.clone();
        for update_node_id in nodes.iter() {
            let node_props = props.node_props.get_mut(update_node_id).unwrap();
            self.update_node(device, queue, *update_node_id, node_props);
        }
    }

    /// Paint the given render target.
    /// The most recent call to `update` will dictate
    /// what graph nodes and render targets are available.
    /// Every node from the last update will be painted,
    ///  and the resulting textures will be returned, indexed by NodeId.
    /// Typically, but not always, the resulting texture will have a resolution matching the render target resolution.
    /// (the render target resolution is just a hint, and nodes may return what they please.)
    pub fn paint(
        &mut self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        encoder: &mut wgpu::CommandEncoder,
        render_target_id: RenderTargetId,
    ) -> HashMap<NodeId, ArcTextureViewSampler> {
        // Ask the nodes to paint in topo order. Return the resulting textures in a hashmap by node id.
        let mut result: HashMap<NodeId, ArcTextureViewSampler> = HashMap::new();

        let node_ids: Vec<NodeId> = self.graph.nodes.clone();
        for paint_node_id in &node_ids {
            let input_nodes = self.graph_input_mapping.get(paint_node_id).unwrap();
            let input_textures: Vec<Option<ArcTextureViewSampler>> = input_nodes
                .iter()
                .map(|maybe_id| maybe_id.as_ref().map(|id| result.get(id).unwrap().clone()))
                .collect();

            let output_texture = self.paint_node(
                device,
                queue,
                encoder,
                *paint_node_id,
                render_target_id,
                &input_textures,
            );

            result.insert(*paint_node_id, output_texture);
        }

        result
    }

    /// Get a blank (transparent) texture
    pub fn blank_texture(&self) -> &ArcTextureViewSampler {
        &self.blank_texture
    }

    /// Retrieve the current render targets as HashMap of id -> `RenderTargetState`
    pub fn render_target_states(&self) -> &HashMap<RenderTargetId, RenderTargetState> {
        &self.render_target_states
    }

    /// Get the state associated with a given render target
    pub fn render_target_state(&self, id: RenderTargetId) -> Option<&RenderTargetState> {
        self.render_target_states.get(&id)
    }

    /// Load content from disk or the library or something
    pub fn fetch_content(&self, filename: &str) -> io::Result<String> {
        fs::read_to_string(self.resource_dir.join(filename))
    }

    /// Load content from disk or the library or something as raw bytes
    pub fn fetch_content_bytes(&self, filename: &str) -> io::Result<Vec<u8>> {
        fs::read(self.resource_dir.join(filename))
    }

    /// Get all node states
    pub fn node_states(&self) -> &HashMap<NodeId, NodeState> {
        &self.node_states
    }

    /// Get the state associated with a given node
    pub fn node_state(&self, id: NodeId) -> Option<&NodeState> {
        self.node_states.get(&id)
    }
}

impl RenderTargetState {
    /// Return the width of this render target
    pub fn width(&self) -> u32 {
        self.width
    }

    /// Return the height of this render target
    pub fn height(&self) -> u32 {
        self.height
    }

    /// Return the timestep of this render target
    pub fn dt(&self) -> f32 {
        self.dt
    }

    /// Get a texture whose resolution matches the render target resolution
    /// and whose pixel data is white noise (in all four channels)
    pub fn noise_texture(&self) -> &ArcTextureViewSampler {
        &self.noise_texture
    }
}
