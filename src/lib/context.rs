use std::collections::HashMap;
use std::sync::Arc;
use crate::graph::{NodeId, Graph, NodeProps, GlobalProps};
use crate::render_target::{RenderTargetId, RenderTarget, RenderTargetList};
use crate::effect_node::{EffectNodeState};
use rand::Rng;
use std::fs;
use std::io;

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

/// A `Context` bundles all of the state and graphics resources
/// necessary to support iterated rendering of a `Graph`.
/// 
/// `Graph` and `RenderTargetList` are intentionally kept stateless
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
    // Graphics resources
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    blank_texture: ArcTextureViewSampler,

    // Cached props from the last update()
    global_props: GlobalProps,

    // State of individual render targets and nodes
    render_target_states: HashMap<RenderTargetId, RenderTargetState>,
    node_states: HashMap<NodeId, NodeState>,

    // Helpful state related to graph rendering
    node_topo_order: Vec<NodeId>,
    node_inputs: HashMap<NodeId, Vec<Option<NodeId>>>,
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
pub enum NodeState {
    EffectNode(EffectNodeState),
}

impl Context {
    fn create_blank_texture(device: &wgpu::Device, queue: &wgpu::Queue) -> ArcTextureViewSampler {
        let texture_size = wgpu::Extent3d {
            width: 1,
            height: 1,
            depth_or_array_layers: 1,
        };
        let texture = device.create_texture(
            &wgpu::TextureDescriptor {
                size: texture_size,
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8UnormSrgb,
                usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
                label: Some("blank texture"),
            }
        );

        queue.write_texture(
            wgpu::ImageCopyTexture {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &[0, 0, 0, 0],
            wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: std::num::NonZeroU32::new(4),
                rows_per_image: std::num::NonZeroU32::new(1),
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = device.create_sampler(
            &wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Nearest,
                min_filter: wgpu::FilterMode::Nearest,
                mipmap_filter: wgpu::FilterMode::Nearest,
                ..Default::default()
            }
        );

        ArcTextureViewSampler::new(texture, view, sampler)
    }

    fn create_noise_texture(device: &wgpu::Device, queue: &wgpu::Queue, width: u32, height: u32) -> ArcTextureViewSampler {
        // XXX this creates a blank texture!

        let texture_size = wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        };
        let texture = device.create_texture(
            &wgpu::TextureDescriptor {
                size: texture_size,
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8UnormSrgb,
                usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
                label: Some("noise texture"),
            }
        );

        let random_bytes: Vec<u8> = (0 .. width * height * 4).map(|_| { rand::thread_rng().gen::<u8>() }).collect();

        queue.write_texture(
            wgpu::ImageCopyTexture {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &random_bytes,
            wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: std::num::NonZeroU32::new(4 * width),
                rows_per_image: std::num::NonZeroU32::new(height),
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = device.create_sampler(
            &wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Nearest,
                min_filter: wgpu::FilterMode::Nearest,
                mipmap_filter: wgpu::FilterMode::Nearest,
                ..Default::default()
            }
        );

        ArcTextureViewSampler::new(texture, view, sampler)
    }

    /// Create a new context with the given graphics resources.
    /// dt is the expected period (in seconds) with which update() will be called.
    /// This is distinct from the period at which paint() may be called,
    /// and the period for that is set in the render target.
    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {
        let blank_texture = Self::create_blank_texture(&device, &queue);
        Self {
            device,
            queue,
            blank_texture,
            global_props: Default::default(),
            render_target_states: Default::default(),
            node_states: Default::default(),
            node_topo_order: Default::default(),
            node_inputs: Default::default(),
        }
    }

    fn new_render_target_state(self: &Self, render_target: &RenderTarget) -> RenderTargetState {
        RenderTargetState {
            width: render_target.width(),
            height: render_target.height(),
            dt: render_target.dt(),
            noise_texture: Self::create_noise_texture(&self.device, &self.queue, render_target.width(), render_target.height()),
        }
    }

    fn new_node_state(self: &Self, node_props: &NodeProps) -> NodeState {
        match node_props {
            NodeProps::EffectNode(props) => NodeState::EffectNode(EffectNodeState::new(self, props)),
        }
    }

    fn update_node(self: &mut Self, node_id: NodeId, node_props: &mut NodeProps) {
        let mut node_state = self.node_states.remove(&node_id).unwrap();
        match node_state {
            NodeState::EffectNode(ref mut state) => {
                match node_props {
                    NodeProps::EffectNode(ref mut props) => state.update(self, props),
                    //_ => panic!("Type mismatch between props and state"),
                }
            },
        };
        self.node_states.insert(node_id, node_state);
    }

    fn paint_node(self: &mut Self, command_buffers: &mut Vec<wgpu::CommandBuffer>, node_id: NodeId, render_target_id: RenderTargetId, input_textures: &[Option<ArcTextureViewSampler>]) -> ArcTextureViewSampler {
        let mut node_state = self.node_states.remove(&node_id).unwrap();
        let result = match node_state {
            NodeState::EffectNode(ref mut state) => state.paint(self, command_buffers, render_target_id, input_textures),
        };
        self.node_states.insert(node_id, node_state);
        result
    }
    /// Update the internal state of `Context` to match the given graph.
    /// `update` tries to run quickly.
    /// If a newly added node needs time to load resources and initializing,
    /// the node will likely do this asynchronously.
    /// In the meantime, it will probably pass through its input unchanged or return a blank texture.
    /// The exception to this rule is when render targets are added;
    /// node-independent render target initialization (such as generating the noise texture)
    /// happens synchronously, so the `update` call may take a long time
    /// and rendering may stutter.
    ///
    /// The passed in Graph will be mutated to advance its contents by one timestep.
    /// It can then be further mutated before calling update() again
    /// (or replaced entirely.)
    pub fn update(&mut self, graph: &mut Graph, render_targets: &RenderTargetList) {
        // 1. Sample-and-hold global props like `time` and `dt`, then update them
        {
            self.global_props = graph.global_props().clone();
            let props = graph.global_props_mut();
            props.time = (props.time + props.dt) % MAX_TIME;
        }

        // 2. Prune render_target_states and node_states of any nodes or render_targets that are no longer present in the given graph/render_targets

        self.render_target_states.retain(|id, _| render_targets.render_targets().contains_key(id));
        self.node_states.retain(|id, _| graph.contains_node(id));

        // 3. Construct any missing render_target_states or node_states (this may kick of background processing)

        for (check_render_target_id, render_target) in render_targets.render_targets().iter() {
            if !self.render_target_states.contains_key(check_render_target_id) {
                self.render_target_states.insert(*check_render_target_id, self.new_render_target_state(render_target));
            }
        }

        for check_node_id in graph.iter_nodes() {
            if !self.node_states.contains_key(check_node_id) {
                self.node_states.insert(
                    *check_node_id,
                    self.new_node_state(graph.node_props(check_node_id).unwrap())
                );
            }
        }

        // 4. Update state on every node
        let nodes: Vec<NodeId> = graph.iter_nodes().cloned().collect();
        for update_node_id in nodes.iter() {
            let node_props = graph.node_props_mut(update_node_id).unwrap();
            self.update_node(*update_node_id, node_props);
        }

        // 5. Store topo order & input mapping for rendering
        self.node_inputs = graph.input_mapping().clone();
        self.node_topo_order = graph.topo_order().cloned().collect();
    }

    /// Paint the given render target.
    /// The most recent call to `update` will dictate
    /// what graph nodes and render targets are available.
    /// Every node from the last update will be painted,
    ///  and the resulting textures will be returned, indexed by NodeId.
    /// Typically, but not always, the resulting texture will have a resolution matching the render target resolution.
    /// (the render target resolution is just a hint, and nodes may return what they please.)
    pub fn paint(&mut self, command_buffers: &mut Vec<wgpu::CommandBuffer>, render_target_id: RenderTargetId) -> HashMap<NodeId, ArcTextureViewSampler> {
        // Ask the nodes to paint in topo order. Return the resulting textures in a hashmap by node id.
        let mut result: HashMap<NodeId, ArcTextureViewSampler> = HashMap::new();

        let node_ids: Vec<NodeId> = self.node_topo_order.clone();
        for paint_node_id in &node_ids {

            let input_nodes = self.node_inputs.get(paint_node_id).unwrap();
            let input_textures: Vec<Option<ArcTextureViewSampler>> = input_nodes.iter().map(
                |maybe_id| match maybe_id {
                    Some(id) => Some(result.get(id).unwrap().clone()),
                   None => None,
                }
            ).collect();

            let output_texture = self.paint_node(command_buffers, *paint_node_id, render_target_id, &input_textures);

            result.insert(*paint_node_id, output_texture);
        }

        result
    }

    /// Get the WGPU device associated with this context
    pub fn device(&self) -> &Arc<wgpu::Device> {
        &self.device
    }

    /// Get the WGPU queue associated with this context
    pub fn queue(&self) -> &Arc<wgpu::Queue> {
        &self.queue
    }

    /// Get a blank (transparent) texture
    pub fn blank_texture(&self) -> &ArcTextureViewSampler {
        &self.blank_texture
    }

    /// Get the cached global props, based on the last call to update
    pub fn global_props(&self) -> &GlobalProps {
        &self.global_props
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
        fs::read_to_string(filename)
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