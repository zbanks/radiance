use std::collections::HashMap;
use std::sync::Arc;
use crate::graph::{NodeId, Graph, NodeProps};
use crate::chain::{ChainId, Chain, Chains};
use crate::effect_node::{EffectNodeState};

#[derive(Clone)]
pub struct ArcTextureViewSampler {
    pub texture: Arc<wgpu::Texture>,
    pub view: Arc<wgpu::TextureView>,
    pub sampler: Arc<wgpu::Sampler>,
}

impl ArcTextureViewSampler {
    pub fn new(texture: wgpu::Texture, view: wgpu::TextureView, sampler: wgpu::Sampler) -> Self {
        Self {
            texture: Arc::new(texture),
            view: Arc::new(view),
            sampler: Arc::new(sampler),
        }
    }
}

pub struct Context {
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    blank_texture: ArcTextureViewSampler,

    chain_states: HashMap<ChainId, ChainState>,
    node_states: HashMap<NodeId, NodeState>,
}

pub struct ChainState {
    noise_texture: ArcTextureViewSampler,
}

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
                label: Some("noise texture"),
            }
        );

        let random_bytes: Vec<u8> = (0 .. width * height * 4).map(|_| { rand::random::<u8>() }).collect();

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

    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {
        let blank_texture = Self::create_blank_texture(&device, &queue);
        Self {
            device,
            queue,
            blank_texture,
            chain_states: HashMap::new(),
            node_states: HashMap::new(),
        }
    }

    fn new_chain_state(self: &Self, chain: &Chain) -> ChainState {
        ChainState {
            noise_texture: Self::create_noise_texture(&self.device, &self.queue, chain.width(), chain.height()),
        }
    }

    fn new_node_state(self: &Self, node_props: &NodeProps) -> NodeState {
        match node_props {
            NodeProps::EffectNode(props) => NodeState::EffectNode(EffectNodeState::new(self, props)),
        }
    }

    fn paint_node(self: &mut Self, node_id: NodeId, chain_id: ChainId, node_props: &NodeProps, time: f32) -> ArcTextureViewSampler {
        let mut node_state = self.node_states.remove(&node_id).unwrap();
        let result = match node_state {
            NodeState::EffectNode(ref mut state) => {
                match node_props {
                    NodeProps::EffectNode(props) => state.paint(self, chain_id, props, time),
                    _ => panic!("Type mismatch between props and state"),
                }
            },
        };
        self.node_states.insert(node_id, node_state);
        result
    }

    pub fn paint(&mut self, graph: &Graph, chains: &Chains, chain_id: ChainId, time: f32) -> HashMap<NodeId, ArcTextureViewSampler> {
        // 1. Prune chain_states and node_states of any nodes or chains that are no longer present in the given graph/chains
        // 2. Construct any missing chain_states or node_states (this may kick of background processing)
        // 3. Topo-sort graph.
        // 4. Ask the nodes to paint in topo order. Return the resulting textures in a hashmap by node id.

        // 1. Prune chain_states and node_states of any nodes or chains that are no longer present in the given graph/chains

        // TODO

        // 2. Construct any missing chain_states or node_states (this may kick of background processing)

        for (check_chain_id, chain) in chains.chains().iter() {
            if !self.chain_states.contains_key(check_chain_id) {
                self.chain_states.insert(*check_chain_id, self.new_chain_state(chain));
            }
        }

        for (check_node_id, node_props) in graph.nodes().iter() {
            if !self.node_states.contains_key(check_node_id) {
                self.node_states.insert(*check_node_id, self.new_node_state(node_props));
            }
        }
        // TODO

        // 3. Topo-sort graph.

        // TODO

        // 4. Ask the nodes to paint in topo order. Return the resulting textures in a hashmap by node id.

        let mut result = HashMap::new();

        for (paint_node_id, node_props) in graph.nodes().iter() {
            let output_texture = self.paint_node(*paint_node_id, chain_id, node_props, time);
            result.insert(*paint_node_id, output_texture);
        }

        result
    }

    pub fn blank_texture(&self) -> &ArcTextureViewSampler {
        &self.blank_texture
    }

    pub fn chain_state(&self, id: ChainId) -> Option<&ChainState> {
        self.chain_states.get(&id)
    }
}

impl ChainState {
    pub fn noise_texture(&self) -> &ArcTextureViewSampler {
        &self.noise_texture
    }
}
